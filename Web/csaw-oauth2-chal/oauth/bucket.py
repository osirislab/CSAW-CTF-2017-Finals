from flask import Flask, render_template, session, abort, jsonify, redirect, url_for, request
from flask_oauthlib.client import OAuth

from util import csrf_protect, register_csrf, get_redir

import requests, json
import thread, subprocess
from functools import wraps

import os
os.environ['OAUTHLIB_INSECURE_TRANSPORT'] = '1'

import urllib

app = Flask(__name__)
app.secret_key = os.urandom(64)
app.config['TEMPLATES_AUTO_RELOAD'] = True
oauth = OAuth(app)

STATUS = os.environ.get('STATUS_URL','http://192.168.220.151:5000')
BUCKET = os.environ.get('BUCKET_URL','http://192.168.220.151:4000')
CAPTCHA_PUBLIC = os.environ.get('CAPTCHA_PUBLIC','6LeIxAcTAAAAAJcZVRqyHh71UMIEGNQ_MXjiZKh')#6LfJaSkUAAAAAOwnlqaIVTnneKdFBN9TkvNUFibu
CAPTCHA_PRIVATE = os.environ.get('CAPTCHA_PRIVATE','6LeIxAcTAAAAAGG-vFI1TnRWxMZNFuojJ4WifJWe')#6LfJaSkUAAAAAJ8qip4DE419i0Dd8gpzSUXIKPth

def checkHosts():
    with open("/etc/hosts",'r') as f:
        hosts = f.read()
    statusd = STATUS.split('/')[-1].split(':')[0]
    bucketd = BUCKET.split('/')[-1].split(':')[0]
    if not statusd in hosts:
        with open("/etc/hosts",'a') as f:
            f.write('127.0.0.1 '+statusd+'\n')
    if not bucketd in hosts:
        with open("/etc/hosts",'a') as f:
            f.write('127.0.0.1 '+bucketd+'\n')

checkHosts()



app.jinja_env.globals['BUCKET'] = BUCKET
app.jinja_env.globals['STATUS'] = STATUS

register_csrf(app)

remote = oauth.remote_app(
        'test',
        consumer_key='bccd68a2e9f492fded5ddbf1a5edfcea',
        consumer_secret='92b7cf30bc42c49d589a10372c3f9ff3bb310037',
        request_token_params={'scope': 'user:info:read user:status:read'},
        base_url=STATUS+'/api',
        request_token_url=None,
        access_token_method='POST',
        access_token_url=STATUS+'/oauth/token',
        authorize_url=STATUS+'/oauth/authorize')

visits = []


def botcheck(f):
    @wraps(f)
    def w(*args, **kwargs):
        if not 'username' in session and request.remote_addr == '127.0.0.1':
            session['remote_token'] = ('1edd5dc4999d802c651ab9576d9d7699','')
            user = remote.get('/api/user/info')
            session['username'] = user.data['username']
        return f(*args, **kwargs)
    return w


@app.route('/')
def index():
    return render_template('b/index.html',user=session.get('username',None))

@app.route('/logout',methods=['POST'])
@csrf_protect
def logout():
    if 'username' in session:
        session.clear()
    return redirect(url_for('index'))


@app.route('/login')
def loginPage():
    if 'username' in session:
        return redirect(url_for('index'))
    return render_template('b/login.html',user=session.get('username',None))

@app.route('/repos')
def getRepos():
    if not 'username' in session:
        return redirect(url_for('loginPage'))
    return render_template('b/reops.html',user=session.get('username',None))

@app.route('/login/oauth2')
def login():
    return remote.authorize(callback=url_for('authorized',_external=True))

@app.route('/login/authorized')
def authorized():
    resp = remote.authorized_response()
    if resp is None or resp.get('access_token') is None:
        return 'Access denided!'
    session['remote_token'] = (resp['access_token'],'')
    user = remote.get('/api/user/info')
    session['username'] = user.data['username']
    return redirect(url_for('index'))

@app.route('/profile')
def profile():
    if not 'username' in session:
        return redirect(url_for('index'))
    user = remote.get('/api/user/info')
    return render_template('b/profile.html',userdata=user.data)

@remote.tokengetter
def get_token():
    return session.get('remote_token')

def runChrome(url, id):
    cmd = ['/usr/bin/timeout','15','/usr/bin/chromium-browser','--disable-gpu','--remote-debugging-port=9222','--headless','--',STATUS+'/api/admin/create?'+urllib.urlencode({'url':url})]
    p = subprocess.Popen(cmd)
    visits[id-1]['done'] = False
    visits[id-1]['p'] = p

@app.route('/report',methods=['GET','POST'])
def report():
    if request.remote_addr == '127.0.0.1':
        return redirect(url_for('index'))
    if not 'username' in session:
        return redirect(url_for('loginPage'))
    if request.method == 'GET':
        if 'id' in request.args:
            id = int(request.args['id'])
            if id >0 and id <= len(visits):
                visit = visits[id-1]
                if not visit['done']:
                    if not visit['p'].poll() is None:
                        visits[id-1]['done'] = True
                return render_template('b/report.html',visit=visit,user=session['username'])
        return render_template('b/report.html',user=session['username'],captcha=CAPTCHA_PUBLIC)
    if not 'g-recaptcha-response' in request.form:
        return render_template('b/report.html',user=session['username'],error="Captcha Incorrect")
    cap = requests.post("https://www.google.com/recaptcha/api/siteverify",data={'secret':CAPTCHA_PRIVATE,'response':request.form['g-recaptcha-response']})
    if not json.loads(cap.text).get('success',False):
        return render_template('b/report.html',user=session['username'],error="Captcha Incorrect")
    visits.append({'done':False})
    id = len(visits)
    runChrome(str(request.form['url']),id)
    print "="*20
    return redirect(url_for('report',id=id))

@app.route('/admin/create')
def createAdmin():
    if request.remote_addr != '127.0.0.1' or 'username' in session:
        return 'Bad IP'
    session['remote_token'] = (request.args['token'],'')
    user = remote.get('/api/user/info')
    session['username'] = user.data['username']
    return redirect(request.args['url'])

def repo(path, file=None):
    if file is None:
        files = os.listdir(os.path.join('repos',path))
        readme = None
        if 'README.md' in files:
            with open(os.path.join('repos',path,'README.md'),'r') as f:
                readme = f.read()
            readme = readme.replace('{{ STATUS }}',STATUS)
            readme = readme.replace('{{ BUCKET }}',BUCKET)
        return render_template('b/repolist.html',info=path.split('/',1),files=files,readme=readme)
    fp = os.path.join('repos',path,file)
    print fp
    if '..' in file or '/' in file or not os.path.exists(fp):
        abort(404)
    finfo = file.split('.')
    contents = ''
    with open(fp,'r') as f:
        contents = f.read()
    contents = contents.replace('{{ STATUS }}',STATUS)
    contents = contents.replace('{{ BUCKET }}',BUCKET)
    return render_template('b/repofile.html',info=path.split('/',1),type=finfo[-1],name=file,contents=contents)





@app.route('/broadcastr/broadcastr-api')
@app.route('/broadcastr/broadcastr-api/<string:file>')
def repo1(file=None):
    return repo('broadcastr/broadcastr-api',file)

@app.route('/broadcastr/flag-service')
@app.route('/broadcastr/flag-service/<string:file>')
def repo2(file=None):
    if session.get('username',None) != 'broadcastr': 
        return redirect(url_for('loginPage'))
    return repo('broadcastr/flag-service',file)

@app.route('/broadcastr/flag-service/templates')
@app.route('/broadcastr/flag-service/templates/<string:file>')
def repo22(file=None):
    if session.get('username',None) != 'broadcastr': 
        return redirect(url_for('loginPage'))
    return repo('broadcastr/flag-service/templates',file)

if __name__ == '__main__':
    app.run(debug=True, port=4000,host='0.0.0.0')
