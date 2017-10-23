import os,time,json
from flask import Flask, render_template, session, abort, jsonify, redirect, url_for, request, Response

app = Flask(__name__)
app.secret_key = os.urandom(32)

@app.route('/')
def index():
    if request.remote_addr != '127.0.0.1':
        abort(403)
    if not session.get('admin', False):
        return redirect(url_for('login',_redirect=url_for('index')))
    return render_template('index.html',
            flag=session.get('flag',None), token=session['csrftoken'])

@app.route('/login', methods=['GET','POST'])
def login():
    if request.remote_addr != '127.0.0.1':
        abort(403)
    session['_redirect'] = request.args.get('_redirect',url_for('index'))
    if request.method == 'GET':
        print session
        return render_template('login.html')
    if request.form['user'] == 'admin' and request.form['pass'] == 'd6e2dec0eb3394a':
        session['admin'] = True
        session['csrftoken'] = os.urandom(16).encode('hex')
        return redirect(session.get('_redirect', url_for('index')))

    session['last_failed_login'] = '%u with %s'%(time.time(), request.form['user'])
    return render_template('login.html', error='Could not log you in')

@app.route('/flag',methods=['GET','POST'])
def flag():
    if request.remote_addr != '127.0.0.1':
        abort(403)
    if not session.get('admin',False):
        abort(403)
    if not request.form.get('token','') == session.get('csrftoken'):
        abort(403)
    with open("/oauth/flag.txt",'r') as flag:
        session['flag'] = flag.read().strip()
    return redirect(url_for('index'))

@app.route('/debug')
def debug():
    if request.remote_addr != '127.0.0.1':
        abort(403)
    data = ', '.join(str(k)+' '+str(v) for k,v in 
            sorted(session.items(),key=lambda x:x[0]))
    return Response(data, mimetype='text/plain')


if __name__ == '__main__':
    app.run(port=3000,host='127.0.0.1')
