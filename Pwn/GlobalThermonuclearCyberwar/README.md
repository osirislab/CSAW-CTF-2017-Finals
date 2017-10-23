# Global Thermonuclar Cyberwar

It's a Wargames-themed challenge, in 8086 real-mode assembly!

Launch CyberNukes to win... kinda


2-parter:
Part 1:
    Name: DEFCON 1
    Description: The year is 1981. Matthew Cyber-Broderick (You) finds a bizzare system. Understand it, and decrypt the secret ROM within.
    Category: Reverseing
    To solve: Decrypt the payload, the flag is in the ROM
    Points: 50
    Flag: `flag{ok_you_decrypted_it_now_plz_pwn_it!}`
Part 2:
    Name: Global Thermonuclear Cyberwar
    Description: In this strange game, the only winning move is pwn.
    Category: Pwn
    To solve: See `solve.py` for exploit
    Points: 400
    Flag: `flag{c4n_4ny0n3_really_w1n_1n_cyb3rw4r?}`


For both parts, please distribute `cyberwar.rom`, which has the second flag scrubbed.
Run `server.rom` on QEMU with the VNC server exposed to the competitors.

The competitors should be told they can run the binary with:
    `qemu-system-i386 -drive format=raw,file=cyberwar.rom`
And a vnc server locally with:
    `qemu-system-i386 -vnc :0 -drive format=raw,file=cyberwar.rom`

To be nice to the competitors, note that the gdbstub in the latest QEMU on ubuntu has had issues for us. A known-good version of QEMU is:
    `2.10.1`
