from socket import socket
from random import randint
from base64 import b64decode

def gena():
    return randint(0, 1024)

p = 251
g = 6

class Client:

    class Connection:
        def __init__(self, host, port, cookies=None):
            self._sock = socket()
            self._sock.connect( (host, port) )
            self._cookies = cookies if cookies else []
            self._diffie_hellman()

        def _diffie_hellman(self):
            a = gena()
            A = (g ** a) % p
            self._sock.send(bytes([A]))
            B = ord(self._sock.recv(1))
            self._s = ((B % p) ** a) % p

        def send(self, msg):
            data = [msg, *('{}:{}'.format(*cookie) for cookie in self._cookies)]
            # print(data)
            _data = '\t'.join(data)
            _msg = bytes([ ord(c) ^ self._s for c in _data ])
            self._sock.send(_msg)

        def recv(self, n):
            res = self._sock.recv(n)
            res = bytes([ c ^ self._s for c in res])
            return res

        def recvitem(self):
            _type = self.recv(1).decode('utf-8')
            _size = b''
            _byte = self.recv(1)
            while True:
                _size += _byte
                _byte = self.recv(1)
                if _byte == b'\r':
                    _size += _byte
                    _byte = self.recv(1)
                    if _byte == b'\n':
                        _size = _size[:-1]
                        break
            _size = int.from_bytes(_size, byteorder='big')
            _data = self.recv(_size) # item
            _delm = self.recv(2) # carriage return
            return _type, _size, _data

    def __init__(self):
        pass

    def connect(self, host, port, cookies=None):
        _sock = self.Connection(host, port, cookies)
        # print(host, port)
        return _sock

    def retrieve_from(self, host, port, selector, cookies=None):
        req = self.connect(host, int(port), cookies)
        req.send(selector)
        print('sent', selector)
        _type, _size, _desc = req.recvitem()
        print(_desc)
        if _type == 'P':
            # print(_desc)
            _listings = _desc.decode('utf-8').split('\r\n')
            listings = []
            for listing in _listings:
                split_on = listing.index('\t')
                _ltype = listing[:split_on]
                _ldesc = listing[split_on + 1:]
                if _ltype not in ['T', 'E', 'C']:
                    _selector, _host, _port, *_kvpairs = _ldesc.split()
                    yield from self.retrieve_from(_host, _port, _selector, _kvpairs)
                else:
                    yield _ltype, _ldesc
        else:
            yield _type, _desc

    def retrieve(self, host, port, selector='/', cookies=None):
        items = list(self.retrieve_from(host, port, selector, cookies))
        print(items)
        if items[0] in ['G', 'I', 'S', 'F']:
            yield items
        for _type, _desc in items:
            yield _type, _desc
            if _type not in ['T', 'E', 'C']:
                _selector, _host, _port, *_kvpairs = _desc.split()
                yield from map(lambda x: ['-'] + list(x), self.retrieve(_host, _port, _selector, _kvpairs))


if __name__ == '__main__':
    client = Client()
    items = list(client.retrieve('web.chal.csaw.io', 433))
    cookies = []
    response = []
    for item in items:
        if item[0] == 'C':
            cookies.append(item[1].split(':')[1])
        print('\t'.join(item))
    flag = items[-2][1]
    decoded = [b for b in b64decode(flag)]
    for i, cookie in enumerate(cookies):
        fb = decoded[i % len(decoded)]
        decoded[i % len(decoded)] = fb ^ int(cookie)
    print(''.join(chr(c) for c in decoded))
    print('finished')

    print('\n'.join(client.retrieve('web.chal.csaw.io', 433, '/imnotabusive')))

