import os
import requests

from flask import Flask, app, request, redirect, render_template, session, url_for, abort, jsonify
from flask_sqlalchemy import SQLAlchemy
from sqlalchemy_utils import database_exists, create_database
from sqlalchemy.engine.url import make_url
from sqlalchemy.exc import OperationalError

from functools import wraps

from models import db, User, Instance

from hashlib import sha256

from itsdangerous import Signer
import pickle
import urllib

app = Flask(__name__)

if os.path.exists('/tmp/key'):
    with open('/tmp/key','rb') as f:
        key = f.read()
else:
    key = os.urandom(32)
    with open('/tmp/key','wb') as f:
        f.write(key)

app.config.update(dict(
    SECRET_KEY=key,
    SQLALCHEMY_DATABASE_URI='sqlite:////tmp/test.db',
    SQLALCHEMY_TRACK_MODIFICATIONS=False,
    TEMPLATES_AUTO_RELOAD=True
))

CTFD_URL = "https://ctf.csaw.io"
SHARED_SECRET = "W0W_Much_Sh@r3d_S3cret"

@app.template_filter('ctime')
def timectime(s):
    return time.ctime(s) # datetime.datetime.fromtimestamp(s)

def startDatabase():
    url = make_url(app.config['SQLALCHEMY_DATABASE_URI'])

    db.init_app(app)
    try:
        if not (url.drivername.startswith('sqlite') or database_exists(url)):
            create_database(url)
        db.create_all()
    except OperationalError:
        db.create_all()
    else:
        db.create_all()
    app.db = db

with app.app_context():
    startDatabase()


def byteify(input):
    if isinstance(input, dict):
        return {byteify(key): byteify(value)
                for key, value in input.iteritems()}
    elif isinstance(input, list):
        return [byteify(element) for element in input]
    elif isinstance(input, unicode):
        return input.encode('utf-8')
    else:
        return input

@app.route('/')
def index():
    return render_template('landing.html')

@app.route('/login',methods=['POST','GET'])
def login():
    if 'user' in session:
        return redirect(url_for('dashboard'))
    if request.method=='GET':
        return render_template('login.html')
    if not 'username' in request.form or not 'password' in request.form:
        return render_template('login.html',error='Could not log you in')
    username = request.form['username']
    password = request.form['password']

    stuff = requests.put(CTFD_URL + '/kws_login', data={
        'username': username,
        'password': password,
    }).json()

    if not stuff['ok']:
        return render_template('login.html',error='Could not log you in')

    u = User.query.filter_by(name=request.form['username']).first()
    if not u:
        u = User(username, password)
        db.session.add(u)
        db.session.commit()
        db.session.refresh(u)
        i = Instance('Instance', stuff['ip'], SHARED_SECRET, u)
        db.session.add(i)
        db.session.commit()

    session['user'] = u.id
    return redirect(url_for('dashboard'))

@app.route('/logout')
def logout():
    if not session.get('user'):
        return redirect(url_for('index'))

    inst = Instance.query.filter_by(user_id=session['user']).first()
    if inst:
        db.session.delete(inst)
        db.session.commit()

    u = User.query.filter_by(id=session['user']).first()
    if u:
        db.session.delete(u)
        db.session.commit()

    session.clear()
    return redirect(url_for('index'))

def authed(f):
    @wraps(f)
    def decorated(*args, **kwargs):
        if 'user' in session:
            user = User.query.filter_by(id=session['user']).first()
            instance = user.insts[0]
            kwargs['user'] = user
            kwargs['instance'] = instance
            return f(*args, **kwargs)
        return redirect(url_for('login'))
    return decorated

@app.route('/dashboard')
@authed
def dashboard(user, instance):
    s = Signer(instance.key)
    return render_template('dashboard.html',user=user, instance=instance,
            objectListRequest=urllib.quote_plus(s.sign('LIST')))

@app.route('/instance')
@authed
def instance(user, instance):
    s = Signer(instance.key)
    return render_template('instance.html',user=user, instance=instance,
            objectListRequest=urllib.quote_plus(s.sign('LIST')))

def apiAuthed(f):
    @wraps(f)
    def decorated(*args, **kwargs):
        if 'user' in session:
            user = User.query.filter_by(id=session['user']).first()
            instance = user.insts[0]
            kwargs['user'] = user
            kwargs['instance'] = instance
            return f(*args, **kwargs)
        abort(403)
    return decorated

@app.route('/api/serialize', methods=['POST'])
@apiAuthed
def serialize(user, instance):
    if 'name' in request.json and 'object' in request.json:
        if type(request.json['name']) != str and type(request.json['name']) != unicode:
            abort(400)

        name = str(request.json['name'].replace('.','_'))
        data = pickle.dumps(byteify(request.json['object']))
        s = Signer(instance.key)
        signed = s.sign('STORE.'+name+'.'+data)
        return jsonify(action=signed)
    abort(400)

@app.route('/api/delete', methods=['POST'])
@apiAuthed
def delete(user, instance):
    if 'name' in request.json:
        if type(request.json['name']) != str and type(request.json['name']) != unicode:
            abort(400)
        name = request.json['name'].replace('.','_')
        s = Signer(instance.key)
        signed = s.sign('DELETE.'+name)
        return jsonify(action=signed)
    abort(400)

@app.route('/api/get', methods=['POST'])
@apiAuthed
def get(user, instance):
    if 'name' in request.json:
        if type(request.json['name']) != str and type(request.json['name']) != unicode:
            abort(400)
        name = request.json['name'].replace('.','_')
        s = Signer(instance.key)
        signed = s.sign('GET.json.'+name)
        return jsonify(action=signed)
    abort(400)

@app.route('/api/share', methods=['POST'])
@apiAuthed
def share(user, instance):
    if 'name' in request.json:
        if type(request.json['name']) != str and type(request.json['name']) != unicode:
            abort(400)
        s = Signer(instance.key)
        signed = s.sign(request.json['name']+'.'+user.name.replace('.','_'))
        return jsonify(sig=signed)
    abort(400)


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=6001)
