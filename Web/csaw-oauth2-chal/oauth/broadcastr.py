from datetime import datetime, timedelta

from util import csrf_protect, register_csrf, get_redir
import time

from urlparse import urlparse

import os 
os.environ['OAUTHLIB_INSECURE_TRANSPORT'] = '1'

from hashlib import sha256
import random

from flask import Flask, render_template, session, abort, jsonify, request, redirect, url_for, escape
from flask_oauthlib.provider import OAuth2Provider

from functools import wraps
import urllib

import re

# nosql at its finest
users = {}
followcounts = []
topfollows = []

MAX_TOP = 32

EMAIL_LEN = 32

STATUS = os.environ.get('STATUS_URL','http://192.168.220.151:5000')
BUCKET = os.environ.get('BUCKET_URL','http://192.168.220.151:4000')

SCOPES = {'user:info:read':'Read your profile',
        'user:info:write':'Update your email',
        'user:status:read':'Read your most recent broadcasts',
        'user:history:read':'Read all your broadcasts'}


class User(object):
    maxid = 0
    def __init__(self, name, password, email=''):
        self.id = User.maxid
        User.maxid += 1
        self.username = name
        self.password = password
        self.data = {}
        self.email = email
        self.status = []
        self.apps = set()
        self.tokens = set()
        self.following = set()
        followcounts.append([self.id,0])
        self.follow(0)

    @property
    def follower_count(self):
        if len(followcounts) <= self.id:
            return 0
        return followcounts[self.id][1]

    def follow(self, fid):
        global topfollows
        if not fid in self.following and fid is not self.id:
            self.following.add(fid)
            followcounts[fid][1] += 1
            if not fid in topfollows:
                if len(topfollows) < MAX_TOP:
                    topfollows.append(fid)
                else:
                    topfollows = sorted(topfollows+[fid],
                            key=lambda x: followcounts[x][1], reverse=True)[:MAX_TOP]




def addStatus(user, status):
    if len(user.status) > 31:
        user.status = user.status[-32:]
    user.status.append((status, int(time.time())))

#+SOURCEHIDE
admin = User('broadcastr',os.urandom(32).encode('hex'))
users[admin.id] = admin

addStatus(admin, "Hey everyone! Welcome to Broadcastr! You are now part of the future!")
addStatus(admin, "Had tacos for lunch, one of my favorite snacks!")
addStatus(admin, "Did you know we have an API?")
addStatus(admin, "Here at Broadcastr we use cutting edge technology, such as OAuth2! You know it is good because they made a sequel.")

a = User('a',sha256('a').hexdigest(),'a@a.com')
users[a.id] = a
#-SOURCEHIDE

clients = {}

class Client(object):
    def __init__(self, id, name, desc, redirect_uris, scopes):
        self.client_id = id
        self.name = name
        self.description = desc
        self.is_confidential = False
        self._redirect_uris = redirect_uris
        self._default_scopes = scopes
    @property
    def client_type(self):
        if self.is_confidential:
            return 'confidential'
        return 'public'
    @property
    def redirect_uris(self):
        if self._redirect_uris:
            return self._redirect_uris.split()
        return []
    @property
    def default_redirect_uri(self):
        return self.redirect_uris[0]
    @property
    def default_scopes(self):
        if self._default_scopes:
            return self._default_scopes.split()
        return []
#+SOURCEHIDE
defaultclient = Client('bccd68a2e9f492fded5ddbf1a5edfcea',
        'Source Bucket','Code storage in the cloud',BUCKET+'/login/authorized',
        'user:info:read user:status:read')
clients[defaultclient.client_id] = defaultclient
#-SOURCEHIDE

grants = {}


class Grant(object):
    maxid = 0
    def __init__(self, client_id, code, redirect_uri, scopes, user_id, expires):
        self.id = Grant.maxid
        Grant.maxid += 1
        self.user_id = user_id
        self.client_id = client_id
        self.code = code
        self.redirect_uri = redirect_uri
        self.expires = expires
        self._scopes = scopes

    def delete(self):
        del grants[self.id]

    @property
    def scopes(self):
        if self._scopes:
            return self._scopes.split()
        return []
    @property
    def user(self):
        return users.get(self.user_id, None)

tokens = {}

class Token(object):
    maxid = 0
    def __init__(self, access_token, refresh_token, token_type, scopes, expires, client_id, user_id):
        self.id = Token.maxid
        Token.maxid += 1
        self.client_id = client_id
        self.user_id = user_id
        self.token_type = token_type
        self.access_token = access_token
        self.refresh_token = refresh_token
        self.expires = expires
        self._scopes = scopes

    def delete(self):
        del tokens[self.id]

    @property
    def scopes(self):
        if self._scopes:
            return self._scopes.split()
        return []
    @property
    def user(self):
        return users.get(self.user_id, None)



def require_auth(f):
    @wraps(f)
    def w(*args, **kwds):
        if not session.get('userid',-1) in users:
            session['loginRedir'] = request.full_path
            return redirect(url_for('login',redir='true'))
        return f(*args, **kwds)
    return w

def getUser():
    return users.get(session.get('userid',-1),None)


app = Flask(__name__)
app.secret_key = os.urandom(64)
oauth = OAuth2Provider(app)

app.jinja_env.globals['BUCKET'] = BUCKET
app.jinja_env.globals['STATUS'] = STATUS
app.config['TEMPLATES_AUTO_RELOAD'] = True

register_csrf(app)

@app.errorhandler(404)
def four0four(e):
    return render_template('a/404.html', user=getUser()), 404


@app.route('/')
def index():
    user = getUser()
    if user is None:
        top = [users[x] for x in random.sample(topfollows, min(9,len(topfollows)))]
        top = [(x, x.status[-1]) for x in top if len(x.status)>0]
        return render_template('a/index.html',top=top)

    status = [(users[uid],x) for uid in list(user.following)+[user.id] for x in users[uid].status]
    status.sort(lambda x,y: y[1][1])
    status.reverse()
    if random.randint(0,8) == 0 or True:
        new = [x for x in topfollows if not x in user.following and x != user.id]
        new = [users[x] for x in random.sample(new, min(3,len(new)))]
        return render_template('a/timeline.html',user=user,timeline=status,new=new)
    return render_template('a/timeline.html',user=user,timeline=status)
    try:
        pass
    except:
        return render_template('a/timeline.html',user=user,
        error="<strong>OMG I'M SORRY!!!</strong> For some reason we couldn't fetch your timeline....")

@app.route('/u/<username>')
def viewUser(username):
    user = getUser()
    fuser = next((x for x in users.values() if x.username == str(username)), None)
    if fuser is None:
        abort(404)
    status = [(fuser,x) for x in reversed(fuser.status)]
    return render_template('a/timeline.html',timeline=status, user=user, fuser=fuser)

@app.route('/follow',methods=['GET','POST'])
@require_auth
@get_redir('index')
@csrf_protect
def follow():
    if not request.form.get('id',''):
        abort(400)
    user = getUser()
    try:
        fuser = users.get(int(request.form['id']),None)
        if fuser and not fuser.id in user.following:
            user.follow(fuser.id)
            return redirect(url_for('viewUser',username=fuser.username))
    except:
        pass
    abort(400)

@app.route('/unfollow',methods=['GET','POST'])
@require_auth
@get_redir('index')
@csrf_protect
def unfollow():
    if not request.form.get('id',''):
        abort(400)
    user = getUser()
    try:
        fuser = users.get(int(request.form['id']),None)
        if fuser and fuser.id in user.following:
            user.following.remove(fuser.id)
            followcounts[fuser.id][1] -= 1
            return redirect(url_for('viewUser',username=fuser.username))
    except:
        pass
    abort(400)



@app.route('/login', methods=['GET','POST'])
def login():
    if not getUser() is None:
        return redirect(url_for('index'))
    if request.method == 'GET':
        if not request.args.get('redir','false') == 'true':
            session['loginRedir'] = '/'
        return render_template('a/login.html', reg='reg'in request.args, error=None)
    if 'register' in request.form:
        if (not request.form.get('username','') or not request.form.get('password','') or
                not request.form.get('confpassword','') or not request.form.get('email','')):
            return render_template('a/login.html', reg=True,
                    error='<strong>Hold on there!</strong> You are missing some fields')

        if request.form['password'] != request.form['confpassword']:
            return render_template('a/login.html', reg=True,
                    error='<strong>Slow down there!</strong> Those passwords don\'t match')

        if len(request.form['username']) > EMAIL_LEN:
            return render_template('a/login.html', reg=True,
                    error='<strong>Sorry friend...</strong> Your username is too long')
        if not re.match('^[_a-zA-Z0-9]+$', request.form['username']):
            return render_template('a/login.html', reg=True,
                    error='<strong>Sorry friend...</strong> Your username can only have alphanumerics and underscores')

        if next((x for x in users.values() if x.username == str(request.form['username'])), None):
            fakename = request.form['username']+str(random.randint(1,65525))
            return render_template('a/login.html', reg=True,
                    error='<strong>Blast!</strong> Username already taken! Maybe try %s'%escape(fakename))

        if len(request.form['email']) > 50:
            return render_template('a/login.html', reg=True,
                    error='<strong>Sorry buddy...</strong> Your email is too long')


        user = User(str(request.form['username']), sha256(str(request.form['password'])).hexdigest())
        users[user.id] = user

        return render_template('a/login.html',
                success='<strong>You are on your way!</strong> Successfully registered %s. Please login...'%escape(user.username))

    if not request.form.get('username','') or not request.form.get('password',''):
        return render_template('a/login.html',
                error='<strong>Hold on there!</strong> You are missing some fields')

    user = next((x for x in users.values() if x.username == str(request.form['username'])), None)
    if user is None or user.password != sha256(str(request.form['password'])).hexdigest():
        return render_template('a/login.html',error='<strong>Check yourself!</strong> Could not log you in')
    session['userid'] = user.id
    return redirect(request.script_root+session.get('loginRedir','/'))

@app.route('/logout',methods=['GET','POST'])
@get_redir('index')
@csrf_protect
def logout():
    if 'userid' in session:
        del session['userid']
    return redirect(url_for('index'))

@app.route('/app',methods=['GET','POST'])
@require_auth
@csrf_protect
def createApp():
    if request.method == 'GET':
        return render_template('a/app.html',user=getUser())
    user = getUser()
    if (not request.form.get('name','') or not request.form.get('desc','') or
            not request.form.get('redirect','') or not request.form.get('scope')):
        return render_template('a/app.html', user=user,
            error="<strong>What are you doing?</strong> You are missing fields.....")
    if len(request.form['name']) > 32:
        return render_template('a/app.html', user=user,
            error="<strong>What are you doing?</strong> Application name too long")
    if len(request.form['desc']) > 128:
        return render_template('a/app.html', user=user,
            error="<strong>What are you doing?</strong> Application description too long")
    if len(request.form['redirect']) > 128:
        return render_template('a/app.html', user=user,
            error="<strong>What are you doing?</strong> Redirect url too long")
    u = urlparse(request.form['redirect'])
    if not u.scheme in ['http','https'] or u.netloc == '' or not '.' in u.netloc:
        return render_template('a/app.html', user=user,
            error="<strong>What are you doing?</strong> That doesn't look like a valid url")
    for s in request.form.getlist('scope'):
        if not s in SCOPES:
            return render_template('a/app.html', user=user,
                error="<strong>What are you doing?</strong> Invalid scope %s"%escape(s))

    token = os.urandom(16).encode('hex')
    client = Client(token,str(request.form['name']), str(request.form['desc']),
            str(request.form['redirect']), ' '.join(request.form.getlist('scope')))
    clients[client.client_id] = client
    user.apps.add(client.client_id)
    return render_template('a/app.html', user=user, token=token)

@app.route('/broadcast', methods=['GET','POST'])
@require_auth
@csrf_protect
def broadcast():
    if request.method == 'GET':
        return render_template('a/broadcast.html',user=getUser())

    user = getUser()
    if not request.form.get('status',''):
        return render_template('a/broadcast.html',user=getUser(),
            error="<strong>You can't post nothing!</strong> Your status must have some content")
    if len(request.form['status'])>128:
        return render_template('a/broadcast.html',user=getUser(), status=request.form['status'],
            error="<strong>You sure have a lot to say</strong> Sorry, status is limited to 128 characters")

    addStatus(user, request.form['status'])
    return redirect(url_for('viewUser',username=user.username))

@app.route('/profile',methods=['GET','POST'])
@require_auth
@csrf_protect
def editProfile():
    user = getUser()
    if request.method == 'GET':
        toks = [(tokens[t], clients[tokens[t].client_id]) for t in user.tokens]
        apps = [clients[c] for c in user.apps]
        toks += toks

        return render_template('a/profile.html',user=user, toks=toks, apps=apps, didtoks=True,
                tab=2 if 'apps' in request.args else 0)

    if 'update' in request.form:
        if not request.form.get('email',''):
            return render_template('a/profile.html',user=getUser(), tab=0,
                    error="<strong>How should we contact you?</strong> You are missing your email")
        if len(request.form['email']) > EMAIL_LEN:
            return render_template('a/profile.html',user=getUser(), tab=0,
                    error='<strong>Sorry buddy...</strong> Your email is too long')
        getUser().email = request.form['email']
        return render_template('a/profile.html',user=getUser(), tab=0,
                success='<strong>Good to go!</strong> Your profile is up to date')
    if 'password' in request.form:
        if (not request.form.get('curpassword','') or not request.form.get('password','') or
                not request.form.get('confpassword','')):
            return render_template('a/profile.html',user=getUser(), tab=1,
                    error='<strong>Look again</strong> Missing fields...')
        if sha256(request.form['curpassword']).hexdigest() != user.password:
            return render_template('a/profile.html',user=getUser(), tab=1,
                    error='<strong>Try again...</strong> Wrong password')
        if not request.form['password'] == request.form['confpassword']:
            return render_template('a/profile.html',user=getUser(), tab=1,
                    error='<strong>Hold on there!</strong> Your new passwords don\'t match :(')

        user.password = sha256(request.form['curpassword']).hexdigest()
        return render_template('a/profile.html',user=getUser(), tab=1,
                success='<strong>Good to go!</strong> Your password has been changed!')
    abort(400)













    



@oauth.clientgetter
def load_client(clientId):
    return clients.get(clientId, None)

@oauth.grantgetter
def load_grant(client_id, code):
    return next((x for x in grants.values() if x.client_id == client_id and x.code == code), None)

@oauth.grantsetter
@require_auth
def save_grant(client_id, code, request, *args, **kwargs):
    expires = datetime.utcnow() + timedelta(seconds=100)
    grant = Grant(client_id, code['code'], request.redirect_uri,
            ' '.join(request.scopes), session['userid'],
            expires)
    grants[grant.id] = grant
    return grant

@oauth.tokengetter
def load_token(access_token=None, refresh_token=None):
    if access_token:
        return next((x for x in tokens.values() if x.access_token == access_token), None)
    elif refresh_token:
        return next((x for x in tokens.values() if x.refresh_token == refresh_token), None)

@oauth.tokensetter
def save_token(token, request, *args, **kwargs):
    print "saving token",token
    toks = filter(lambda x: x.client_id == request.client.client_id and x.user_id == request.user.id, tokens.values())
    for t in toks:
        del tokens[t.id]
        users[request.user.id].tokens.remove(t.id)
    expires_in = token.get('expires_in')
    expires = datetime.utcnow() + timedelta(seconds=expires_in)

    tok = Token(token['access_token'], token['refresh_token'], token['token_type'], token['scope'],
            expires, request.client.client_id, request.user.id)
    tokens[tok.id] = tok
    users[request.user.id].tokens.add(tok.id)
    return tok


@app.route('/oauth/authorize', methods=['GET','POST'])
@require_auth
@oauth.authorize_handler
def authorize(*args, **kwargs):
    if request.method == 'GET':
        client_id = kwargs.get('client_id')
        client = clients.get(client_id,None)
        kwargs['client'] = client
        return render_template('a/confirm.html', client=client, scopes=[SCOPES[x] for x in kwargs['scopes']])
    confirm = request.form.get('confirm', 'no')
    return confirm == 'Yes'

@app.route('/oauth/token', methods=['POST','GET']) #methods=['POST'])
@oauth.token_handler
def access_token():
    return {}

@app.route('/api')
def api():
    return jsonify({'endpoints':{
        'user':{
            'info':['/api/user/info','/api/user/info/update'],
            'broadcasts':['/api/user/status','/api/user/history'],
            'followers':['/api/users/followers']}}})

@app.route('/api/user/info')
@oauth.require_oauth('user:info:read')
def getUserInfo():
    user = request.oauth.user
    return jsonify(username=user.username, email=user.email)

@app.route('/api/user/status')
@oauth.require_oauth('user:status:read')
def getUserStatus():
    user = request.oauth.user
    return jsonify(status=user.status[-1] if len(user.status)>0 else None)

@app.route('/api/user/info/update',methods=['POST'])
@oauth.require_oauth('user:info:write')
def setUserInfo():
    user = request.oauth.user
    if 'email' in request.json:
        if len(request.json['email']) > 50:
            return jsonify(email=user.email, username=user.username, error="Email too long")
        user.email = str(request.json['email'])
    return jsonify(email=user.email, username=user.username)

@app.route('/api/user/history')
@oauth.require_oauth('user:history:read')
def getUserHistory():
    return jsonify(history=user.status)

@app.route('/oauth/errors')
def error():
    return jsonify(request.args)

#+SOURCEHIDE
@app.route('/api/admin/create')
def createAdmin():
    if request.remote_addr != '127.0.0.1' or 'userid' in session:
        return 'Bad IP'
    admin = User('broadcastr',os.urandom(32).encode('hex'))
    users[admin.id] = admin
    admin_grant = Grant(defaultclient.client_id, os.urandom(16).encode('hex'), BUCKET+'/login/authorized',
            'user:info:read user:status:read', admin.id, datetime.utcnow() + timedelta(seconds=1000000000))
    grants[admin_grant.id] = admin_grant
    admin_token = Token(os.urandom(16).encode('hex'),os.urandom(16).encode('hex'),
            'bearer', 'user:info:read user:status:read', 
            datetime.utcnow() + timedelta(seconds=1000000000), defaultclient.client_id, admin.id)
    tokens[admin_token.id] = admin_token
    session['userid'] = admin.id

    return redirect(BUCKET+'/admin/create?'+urllib.urlencode({
            'token':admin_token.access_token,
            'url':request.args['url']}))
#-SOURCEHIDE



if __name__ == '__main__':
    app.run(debug=True,host='0.0.0.0')
