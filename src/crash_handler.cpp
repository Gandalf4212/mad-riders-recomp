// Crash reporting - see crash_handler.h.

#include "crash_handler.h"

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <string>

#include <rex/logging.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <dbghelp.h>
#ifdef _MSC_VER
#pragma comment(lib, "dbghelp.lib")
#endif
#endif

namespace crash_handler {

#ifdef _WIN32
namespace {

std::filesystem::path g_exe_dir;
volatile LONG g_in_handler = 0;

const char* ExceptionName(DWORD code) {
  switch (code) {
    case EXCEPTION_ACCESS_VIOLATION: return "access violation";
    case EXCEPTION_ILLEGAL_INSTRUCTION: return "illegal instruction";
    case EXCEPTION_INT_DIVIDE_BY_ZERO: return "integer divide by zero";
    case EXCEPTION_STACK_OVERFLOW: return "stack overflow";
    case EXCEPTION_BREAKPOINT: return "breakpoint (debug trap)";
    case EXCEPTION_PRIV_INSTRUCTION: return "privileged instruction";
    case EXCEPTION_IN_PAGE_ERROR: return "in-page error";
    case 0xE06D7363: return "C++ exception";
    case 0xC0000409: return "stack buffer overrun / fast fail";
  }
  return "exception";
}

// "module!symbol+0x12 (file:line)" for an address, as far as dbghelp can tell.
std::string Describe(HANDLE process, DWORD64 addr) {
  char buf[512];
  std::string out;
  HMODULE mod = nullptr;
  if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                             GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                         reinterpret_cast<LPCSTR>(addr), &mod) && mod) {
    char path[MAX_PATH];
    GetModuleFileNameA(mod, path, MAX_PATH);
    const char* name = strrchr(path, '\\');
    snprintf(buf, sizeof buf, "%s+0x%llx", name ? name + 1 : path,
             (unsigned long long)(addr - reinterpret_cast<DWORD64>(mod)));
    out = buf;
  } else {
    snprintf(buf, sizeof buf, "0x%016llx", (unsigned long long)addr);
    out = buf;
  }
  alignas(SYMBOL_INFO) char sym_buf[sizeof(SYMBOL_INFO) + 256];
  auto* sym = reinterpret_cast<SYMBOL_INFO*>(sym_buf);
  sym->SizeOfStruct = sizeof(SYMBOL_INFO);
  sym->MaxNameLen = 255;
  DWORD64 disp = 0;
  if (SymFromAddr(process, addr, &disp, sym)) {
    snprintf(buf, sizeof buf, "  %s+0x%llx", sym->Name, (unsigned long long)disp);
    out += buf;
    IMAGEHLP_LINE64 line{};
    line.SizeOfStruct = sizeof(line);
    DWORD ldisp = 0;
    if (SymGetLineFromAddr64(process, addr, &ldisp, &line)) {
      const char* f = strrchr(line.FileName, '\\');
      if (!f) f = strrchr(line.FileName, '/');
      snprintf(buf, sizeof buf, "  (%s:%lu)", f ? f + 1 : line.FileName, line.LineNumber);
      out += buf;
    }
  }
  return out;
}

void WriteDump(EXCEPTION_POINTERS* ep, std::string& dump_name) {
  std::filesystem::path p = g_exe_dir / "mad_riders_crash.dmp";
  HANDLE f = CreateFileW(p.wstring().c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, nullptr);
  if (f == INVALID_HANDLE_VALUE) return;
  MINIDUMP_EXCEPTION_INFORMATION mei{GetCurrentThreadId(), ep, FALSE};
  // Small dump: threads, stacks, modules. Not the guest memory arena (gigabytes).
  auto type = static_cast<MINIDUMP_TYPE>(MiniDumpWithThreadInfo | MiniDumpWithUnloadedModules |
                                         MiniDumpWithIndirectlyReferencedMemory);
  if (MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), f, type, &mei, nullptr,
                        nullptr))
    dump_name = p.filename().string();
  CloseHandle(f);
}

LONG WINAPI Filter(EXCEPTION_POINTERS* ep) {
  if (InterlockedExchange(&g_in_handler, 1)) return EXCEPTION_CONTINUE_SEARCH;
  HANDLE process = GetCurrentProcess();
  SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
  SymInitializeW(process, g_exe_dir.wstring().c_str(), TRUE);

  const EXCEPTION_RECORD* er = ep->ExceptionRecord;
  const DWORD64 addr = reinterpret_cast<DWORD64>(er->ExceptionAddress);
  std::string head = std::string("CRASH: ") + ExceptionName(er->ExceptionCode);
  char buf[256];
  snprintf(buf, sizeof buf, " (0x%08lX) on thread %lu", er->ExceptionCode, GetCurrentThreadId());
  head += buf;
  if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && er->NumberParameters >= 2) {
    const char* op = er->ExceptionInformation[0] == 0   ? "reading"
                     : er->ExceptionInformation[0] == 1 ? "writing"
                                                        : "executing";
    snprintf(buf, sizeof buf, ", %s address 0x%016llx", op,
             (unsigned long long)er->ExceptionInformation[1]);
    head += buf;
  }
  // The report goes to the log (when logging is on) and to mad_riders_crash.txt next to the exe (always:
  // logging is off by default).
  std::string report;
  auto emit = [&report](const std::string& line) {
    REXLOG_CRITICAL("{}", line);
    report += line + "\n";
  };
  emit(head);
  emit("  at " + Describe(process, addr));

  // Stack trace from the faulting context.
  CONTEXT ctx = *ep->ContextRecord;
  STACKFRAME64 frame{};
  frame.AddrPC.Offset = ctx.Rip;
  frame.AddrFrame.Offset = ctx.Rbp;
  frame.AddrStack.Offset = ctx.Rsp;
  frame.AddrPC.Mode = frame.AddrFrame.Mode = frame.AddrStack.Mode = AddrModeFlat;
  for (int i = 0; i < 32; i++) {
    if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, GetCurrentThread(), &frame, &ctx,
                     nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr) ||
        !frame.AddrPC.Offset)
      break;
    snprintf(buf, sizeof buf, "  #%02d ", i);
    emit(buf + Describe(process, frame.AddrPC.Offset));
  }
  snprintf(buf, sizeof buf, "  registers: rcx=%016llx rdx=%016llx r8=%016llx r9=%016llx rsp=%016llx",
           (unsigned long long)ctx.Rcx, (unsigned long long)ctx.Rdx, (unsigned long long)ctx.R8,
           (unsigned long long)ctx.R9, (unsigned long long)ep->ContextRecord->Rsp);
  emit(buf);

  std::string dump;
  WriteDump(ep, dump);
  if (!dump.empty()) emit("Crash dump written to " + dump);
  rex::FlushLogging();
  if (FILE* f = _wfopen((g_exe_dir / "mad_riders_crash.txt").wstring().c_str(), L"wb")) {
    fwrite(report.data(), 1, report.size(), f);
    fclose(f);
  }

  std::string msg = "Mad Riders crashed (" + std::string(ExceptionName(er->ExceptionCode)) +
                    ").\n\nThe details are in mad_riders_crash.txt";
  if (!dump.empty()) msg += " and " + dump;
  msg += " next to the exe.";
  MessageBoxA(nullptr, msg.c_str(), "Mad Riders", MB_OK | MB_ICONERROR);
  return EXCEPTION_EXECUTE_HANDLER;  // terminate the process
}

void OnTerminate() {
  REXLOG_CRITICAL("CRASH: std::terminate called (unhandled C++ exception or fatal error)");
  rex::FlushLogging();
  std::abort();
}

void OnExit() {
  REXLOG_INFO("Process exiting (exit() called)");
  rex::FlushLogging();
}

}  // namespace

void install(const std::filesystem::path& exe_dir) {
  static bool once = false;
  g_exe_dir = exe_dir;
  SetUnhandledExceptionFilter(Filter);
  if (once) return;
  once = true;
  // Keep the error dialog ours, not Windows' "has stopped working".
  SetErrorMode(GetErrorMode() | SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
  std::set_terminate(OnTerminate);
  std::atexit(OnExit);
}

#else

void install(const std::filesystem::path&) {}

#endif

}  // namespace crash_handler
