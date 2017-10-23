import random

from functools import wraps
from flask import request, session, abort, url_for, redirect

def csrf_protect(f):
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if request.method == "POST":
            token = session.pop('_csrf_token', None)
            if not token or token != request.form.get('_csrf_token'):
                abort(403)
        return f(*args, **kwargs)
    return decorated_function


def register_csrf(app):
    def generate_csrf_token():
	if '_csrf_token' not in session:
	    session['_csrf_token'] = '%032x'%random.getrandbits(256)
	return session['_csrf_token']

    app.jinja_env.globals['csrf_token'] = generate_csrf_token 

def get_redir(url):
    def w(f):
        @wraps(f)
        def decorated_function(*args, **kwargs):
            if request.method == "GET":
                return redirect(url_for(url))
            return f(*args, **kwargs)
        return decorated_function
    return w
