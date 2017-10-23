#!/bin/bash

source /config.sh

cat > /etc/supervisor/conf.d/supervisord.conf <<EOF
[supervisord]
nodaemon=true

[program:broadcastr]
command=/oauth/env/bin/uwsgi --ini broadcastr.ini
environment=PATH="/oauth/env/bin",STATUS_URL="http://$BROADCASTR_URL$PORT",BUCKET_URL="http://$BUCKET_URL$PORT"
directory=/oauth
stdout_logfile=/dev/stdout
stdout_logfile_maxbytes=0
stderr_logfile=/dev/stderr
stderr_logfile_maxbytes=0

[program:bucket]
command=/oauth/env/bin/uwsgi --ini bucket.ini
environment=PATH="/oauth/env/bin",STATUS_URL="http://$BROADCASTR_URL$PORT",BUCKET_URL="http://$BUCKET_URL$PORT",CAPTCHA_PUBLIC="$CAPTCHA_PUBLIC",CAPTCHA_PRIVATE="$CAPTCHA_PRIVATE"
directory=/oauth
stdout_logfile=/dev/stdout
stdout_logfile_maxbytes=0
stderr_logfile=/dev/stderr
stderr_logfile_maxbytes=0

[program:flag]
command=python flagserver.py
environment=PATH="/oauth/env/bin"
directory=/oauth/repos/broadcastr/flag-service/
stdout_logfile=/dev/stdout
stdout_logfile_maxbytes=0
stderr_logfile=/dev/stderr
stderr_logfile_maxbytes=0

[program:nginx]
command=/usr/sbin/nginx
stdout_logfile=/dev/stdout
stdout_logfile_maxbytes=0
stderr_logfile=/dev/stderr
stderr_logfile_maxbytes=0
EOF
