import struct
B=0x82000000
D=open('image.bin','rb').read()
def u32(a): return struct.unpack('>I',D[a-B:a-B+4])[0]
