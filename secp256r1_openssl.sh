#!/bin/bash


gcc -O2 -fPIC -I /usr/include/python2.7 -c secp256r1_openssl.c -o secp256r1_openssl.o
ld -shared -lcrypto -lpython2.7 secp256r1_openssl.o -o secp256r1_openssl.so
