from socket import socket
from random import randint
from sys import argv

ENCODING = 'utf-8'
LINE_SEP = '\r\n'
ITEM_SEP = '\t'
KV_SEP = ':'

p = 251
g = 6

def diffieHellman(serv):
    a = randint(0, 1024)
    A = (g ** a) % p
    serv.send(bytes([A]))
    B = ord(serv.recv(1))
    return (B ** a) % p

def decodeParams(params):
    for param in params:
        key, value = param.split(KV_SEP)
        yield {
            'key': key,
            'value': value
        }

class Client:
    class Connection:
        def __init__(self, host, port):
            self._sock = socket()
            self._sock.connect( (host, port) )
            self._secret = diffieHellman(self._sock)

        def _encode(self, data):
            return bytes([ord(b) ^ self._secret for b in data])

        def _decode(self, data):
            return bytes([b ^ self._secret for b in data])

        def send(self, selector, params=None):
            params = params if params else []
            data = ITEM_SEP.join([selector, *('{key}:{value}'.format(**param) for param in params)])
            self._sock.send(self._encode(data))

        def recv(self, n):
            return self._decode(self._sock.recv(n))

        def _get_number(self, byteorder='big'):
            size = b''
            byte = self.recv(1)
            while True:
                size += byte
                byte = self.recv(1)
                # would be nice to make this work for any length line sep
                if byte == b'\r':
                    size += byte
                    byte = self.recv(1)
                    if byte == b'\n':
                        break
            return int.from_bytes(size[:-1], byteorder=byteorder)

        def retrieve(self):
            _type = self.recv(1).decode(ENCODING)
            _size = self._get_number()
            _data = self.recv(_size)
            return _type, _size, _data

    def _connect(self, host, port):
        return self.Connection(host, port)

    def item(self, host, port, selector, params=None, resolve_links=False):
        req = self._connect(host, port)
        req.send(selector, params)
        _type, _size, _data = req.retrieve()
        if _type == 'P':
            listings = _data.decode(ENCODING).split(LINE_SEP)
            for listing in listings:
                _type, *_desc = listing.split(ITEM_SEP)
                yield (_type, *_desc)
                if _type not in ['T', 'E', 'C'] and resolve_links:
                    yield from self.item(_desc[1], int(_desc[2]), _desc[0], decodeParams(_desc[3:]), resolve_links)
        else:
            filename = './' + selector.replace('/', '.').strip('.')
            with open(filename, 'wb') as f:
                f.write(_data)
            yield (_type, 'size:' + str(_size), filename)

    def request(self, host, port, selector, params=None, resolve_links=False):
        return '\n'.join('\t'.join(thing) for thing in self.item(host, int(port), '/', resolve_links=True))



if __name__ == '__main__':
    print('working')
    host = argv[1]
    port = int(argv[2])
    client = Client()
    print(client.request(host, port, '/', resolve_links=True))


