#/usr/bin/env python3

ENCKEY = b'-JOSHUA-'

with open('game.bin', 'rb') as f:
    cz = bytearray(f.read())

for i, _ in enumerate(cz):
    cz[i] ^= ENCKEY[i % len(ENCKEY)] ^ 0x3c

with open('game.bin.enc', 'wb') as f:
    f.write(cz)
