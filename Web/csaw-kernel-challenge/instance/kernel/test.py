import fcntl
import array
import struct

f = open("/dev/kws",'w')


def insert(key, value):
    arg = struct.pack('<QQ',id(k),id(v))
    fcntl.ioctl(f.fileno(),1, arg)


def getJson(key):
    arrayLen = 100
    while True:
        out = array.array('b','\0'*arrayLen)
        outAddr, outLen = out.buffer_info()
        arg = struct.pack('<QQQ',outAddr, outLen, id(key))
        try:
            fcntl.ioctl(f.fileno(),4, arg)
            return out.tostring().strip('\0')
        except:
            arrayLen*=2

def delete(key):
    arg = struct.pack('<Q',id(key))
    fcntl.ioctl(f.fileno(),5, arg)


k = "key"
v = {"hello":{"world":0x41424344}}
v = {"a":[]}
insert(k,v)

print repr(getJson("key"))
