# KWS Kernel Exploit Challenge

The kernel module on the KWS instance implments a hashmap, with the ability to parse
python objects out of userspace.

The supported objects are dict, string, and number.

When a dict is parsed from userspace, it assumes it is a json based dict and has a special case
for the keys of the dict. Keys are assumed to be strings or numbers, not other dicts, so 
`deserializePrimative` is used instead of `deserialize`.

The problem is the if else on line 307. If it is not a number type, it assumes it is a string.
If we create a python object with a dict as a key, it will build it as a string.

This in itself isn't too useful, because we could create a malformed string normally,
and have the same results. However, if we give it a real string, but set the type string to "dict",
it will later on try to access the string as a hashtable.

This allows us to control pointers that the kernel module will follow. The place this is useful is
in the makeCircular code (line 96).

If we create a fake bucket in userspace, which points to some kernelspace memory address,
this will write the address of the fake bucket to that kernelspace address. This gives us an
arbitrary write.

With SMEP on, we can attack `core_pattern`. First we find the address of `core_pattern` in `/proc/kallsyms`.
Then we write `|/tmp/a` to that kernel address. This will cause the kernel to execute `/tmp/a` as root whenever a
program dumps core.

All we need to do is write a shell script to get the flag in `/tmp/a` and cause a segfault.

