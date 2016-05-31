#!/bin/bash

svn co -r81 https://github.com/floodyberry/ed25519-donna
gcc -O2 -std=c99 -fPIC -I /usr/include/python2.7 -I ed25519-donna/trunk/ -c ed25519_donna.c -o ed25519_donna.o
ld -shared -lpython2.7 ed25519_donna.o -o ed25519_donna.so
