# KWS

**Cloud Object Storage With Kernel Accleration**

This is a two part challenge. The first part is a ~300 point web challenge. The second is a 500 point kernel pwning challenge.

Each team should get their own instance running the KWS back end. There is a main server that acts as the
face of the web challenge, and will allow players to reboot/reset their instance (if they fuck up the kernel part)

*Instance control is still not implmented yet*

The goal of the first part is to exploit the usage of pickle on signed objects in the backend server to get a shell on the instance.

The instance is running a custom kernel module (source in `./instance/kernel/hash.c`) which implements a hashmap
that can read and store objects json from a python userspace process.

This module can be exploited to EOP to root. The last flag is in /flag

Solutions are in `./solutions`


Description: We developed a much better alternative to AWS. Our high-performance kernel driver gives us unparalleled speed of execution. And we're super-secure!
