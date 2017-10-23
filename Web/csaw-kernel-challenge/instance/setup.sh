#!/bin/bash

USER="kws"

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

apt-get update
apt-get install make gcc linux-headers-$(uname -r) python python-dev python-pip openssh-server sudo
pip install -r box/reqs.txt
echo 0 | tee /proc/sys/kernel/kptr_restrict # Needed to see kallsyms

cat > /etc/rc.local <<EOF
#!/bin/sh -e
#
# rc.local
#
# This script is executed at the end of each multiuser runlevel.
# Make sure that the script will "exit 0" on success or any other
# value on error.
#
# In order to enable or disable this script just change the execution
# bits.
#
# By default this script does nothing.

insmod /hash.ko
chmod 777 /dev/kws
sudo -u $USER /home/$USER/box/run.sh &

exit 0
EOF

useradd -m $USER

mv box /home/$USER/.
chown -R $USER:$USER /home/$USER/box

mv kernel /root/.
chown -R root:root /root/kernel

mv flag /
chmod 000 /flag
chown root:root /flag

cd /root/kernel
make
cp hash.ko /
chmod 444 /hash.ko
chown root:root /hash.ko

cp hash.c /
chmod 444 /hash.c
chown root:root /hash.c

service ssh start



