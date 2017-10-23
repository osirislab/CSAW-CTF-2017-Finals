#!/bin/bash

source /config.sh

if [ ! -f /.dockerenv ]; then
    echo "This is ment to be run in a docker env";
    exit
fi

if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root" 
    exit 1
fi

cat >> /etc/hosts <<EOF
127.0.0.1 $BROADCASTR_URL
127.0.0.1 $BUCKET_URL
EOF

pip install virtualenv
virtualenv env
source env/bin/activate
pip install -r requirements.txt
pip install uwsgi

cat > /oauth/broadcastr.ini <<EOF
[uwsgi]
module = broadcastr:app
master = true
processes = 1
socket = /tmp/broadcastr.sock
chmod-socket = 666
vacuum = true
die-on-term = true
EOF

cat > /oauth/broadcastr_wsgi.py <<EOF
import os
os.environ['BUCKET_URL'] = 'http://$BUCKET_URL$PORT'
os.environ['STATUS_URL'] = 'http://$BROADCASTR_URL$PORT'
from broadcastr import app
EOF


cat > /oauth/bucket.ini <<EOF
[uwsgi]
module = bucket:app
master = true
processes = 1
socket = /tmp/bucket.sock
chmod-socket = 666
vacuum = true
die-on-term = true
EOF

cat > /oauth/bucket_wsgi.py <<EOF
import os
os.environ['BUCKET_URL'] = 'http://$BUCKET_URL$PORT'
os.environ['STATUS_URL'] = 'http://$BROADCASTR_URL$PORT'
from bucket import app
EOF


PORT=${PORT:-:80}

cat > /etc/nginx/sites-available/oauth <<EOF
server {
    listen ${PORT:1} default_server;
    server_name _;

    location / {
        try_files $uri $uri/ =404;
    }
}

server {
    listen ${PORT:1};
    server_name $BROADCASTR_URL;

    location /static  {
        include  /etc/nginx/mime.types;
        alias /oauth/static;
    }

    location / {
        include uwsgi_params;
        uwsgi_pass unix:/tmp/broadcastr.sock;
    }
}

server {
    listen ${PORT:1};
    server_name $BUCKET_URL;

    location /static  {
        include  /etc/nginx/mime.types;
        alias /oauth/static;
    }

    location / {
        include uwsgi_params;
        uwsgi_pass unix:/tmp/bucket.sock;
    }
}
EOF
#TODO add last server

rm /etc/nginx/sites-enabled/default

ln -s /etc/nginx/sites-available/oauth /etc/nginx/sites-enabled
