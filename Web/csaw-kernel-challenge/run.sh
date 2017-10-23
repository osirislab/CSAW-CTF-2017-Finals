#!/bin/sh
qemu-system-x86_64 \
    -hda debian_wheezy_amd64_standard.qcow2 \
    -nographic \
    -monitor /dev/null \
    -net nic,vlan=0 \
    -net user,vlan=0,hostfwd=tcp::2222-:22 \
    -chardev stdio,signal=off,id=serial0 \
    -serial chardev:serial0 \
    -enable-kvm


