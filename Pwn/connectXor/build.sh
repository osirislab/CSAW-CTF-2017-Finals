#!/usr/bin/env bash

set -e

if [ ! -d ./obj ]; then
    mkdir obj
fi

make clean
make release
sudo docker build . -t connect_xor
