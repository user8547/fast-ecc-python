#!/bin/bash

gcc -O2 -fPIC -I /usr/include/python2.7 -c secp256k1_openssl.c -o secp256k1_openssl.o
ld -shared -lcrypto -lpython2.7 secp256k1_openssl.o -o secp256k1_openssl.so
