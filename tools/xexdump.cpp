// xexdump <xex> image   -> writes decrypted/decompressed image (base..end) to stdout-file
#include <image.h>
#include <cstdio>
#include <fstream>
#include <vector>
#include <cstdlib>
int main(int argc, char** argv) {
  std::ifstream f(argv[1], std::ios::binary);
  std::vector<uint8_t> d((std::istreambuf_iterator<char>(f)), {});
  Image img = Image::ParseImage(d.data(), d.size());
  printf("base %zx size %zx sections %zu\n", (size_t)img.base, (size_t)img.size, img.sections.size());
  for (auto& s : img.sections) printf("  %-8s %08zx %08x\n", s.name.c_str(), (size_t)s.base, s.size);
  FILE* o = fopen(argv[2], "wb");
  fwrite(img.data.get(), 1, img.size, o);
  fclose(o);
}
