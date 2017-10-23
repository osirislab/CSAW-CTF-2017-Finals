# Instance Deployment

This code includes the backend of the KWS instances.

This is a flask python server, and the KWS kernel module.

Each team should have their own copy of this box.

The key in `/home/kws/box/key` should be unique per team and match what is in
the main kws database.

# Setup
* Copy this directory to a modern 64 bit ubuntu/debian box (Ideally with kernel security patches :P)
* Run `./setup.sh` on the box
* Delete anything remaining in this directory
* Reboot

You can now take a snapshot of the vm so player can reset if they really fuck something up.

The instances should have at least port 6002 and 22 open

# Flags
The flag for the first part (the web challenge) is in /home/box/flag
The flag for the second part (the kernel challenge) is in /flag
