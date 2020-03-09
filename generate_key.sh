#!/usr/bin/env bash

KEYNAME=rsa256.key

ssh-keygen -t rsa -b 2048 -f $KEYNAME
openssl rsa -in $KEYNAME -pubout -outform PEM -out $KEYNAME.pub
