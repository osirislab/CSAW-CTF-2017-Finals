#!/bin/bash

source config.sh

PORT=${PORT:-:80}

# SYS_ADMIN needed to run chrome
docker run -p ${PORT:1}:${PORT:1} -it --cap-add=SYS_ADMIN oauthchal
