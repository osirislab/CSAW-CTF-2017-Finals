import os

from flask import Flask, app, request, redirect, render_template, session, url_for, abort, jsonify

from functools import wraps

from itsdangerous import Signer
import pickle

from flask_cors import CORS

import fcntl, struct, array

app = Flask(__name__)
cors = CORS(app, resources={r"/action": {"origins": "*"}})


objects = []

with open('./key','rb') as f:
    signkey = f.read()


kws = open('/dev/kws', 'w')


def getList():
    return jsonify(names=objects)


# Store a json object into the kws kernel module
def store(obj):
    obj = obj.split('.',1)
    if len(obj) != 2:
        abort(400)

    key = obj[0]
    value = pickle.loads(obj[1])
    print repr(key)
    print repr(value)
    
    try:
        arg = struct.pack('<QQ', id(key), id(value))
        fcntl.ioctl(kws.fileno(), 1, arg)
        objects.append(key)
        return jsonify(stored=True)
    except:
        return jsonify(stored=False)


# Delete a json object from the kws kernel module
def delete(key):
    if not key in objects:
        abort(404)

    try:
        arg = struct.pack('<Q', id(key))
        fcntl.ioctl(kws.fileno(), 5, arg)
        objects.remove(key)
        return jsonify(deleted=True)
    except:
        return jsonify(deleted=False)



# Have the kws kernel module dump json for the object
def getObject(key):

    if not key in objects:
        abort(404)

    arrayLen = 1000
    while True:
        out = array.array('b', '\0'*arrayLen)
        outAddr, outLen = out.buffer_info()
        arg = struct.pack('<QQQ', outAddr, outLen, id(key))

        try:
            fcntl.ioctl(kws.fileno(), 4, arg)
            return out.tostring().strip('\0')
        except:
            # We need a bigger buffer to hold the json obj
            arrayLen *= 2
            if arrayLen > 0x1000*0x1000:
                return 'Object too large'


@app.route('/action', methods=['POST'])
def action():
    if not 'action' in request.json:
        abort(400)
    s = Signer(signkey)
    try:
        action = s.unsign(request.json['action'])
    except:
        abort(403)

    action = action.split('.',1)
    if action[0] == 'LIST':
        return getList()

    if action[0] == 'GET':
        if len(action) != 2:
            abort(400)
        obj = action[1].split('.',1)
        if len(obj) != 2:
            abort(400)

        key = obj[1]
        return getObject(key)

    if action[0] == 'STORE':
        if len(action) != 2:
            abort(400)
        return store(action[1])

    if action[0] == 'DELETE':
        if len(action) != 2:
            abort(400)
        return delete(action[1])

    abort(400)

@app.route('/share')
def share():
    if not 'sig' in request.args:
        abort(403)
    s = Signer(signkey)
    try:
        action = s.unsign(request.args['sig'])
    except:
        abort(403)

    action = action.split('.',1)
    key = action[0]
    if not key in objects:
        abort(404)

    return getObject(key)
    

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=6002, debug=True)
