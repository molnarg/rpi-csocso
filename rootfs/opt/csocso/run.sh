#!/bin/bash

cd "$(dirname $0)"

while [ 1 ]
do
    python daemon.py >>stdout.log 2>>stderr.log
    sleep 5
done
