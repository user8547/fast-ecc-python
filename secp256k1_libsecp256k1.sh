#!/bin/bash

# sudo apt-get install subversion build-essential libgmp-dev libssl-dev python-dev
svn co -r699 https://github.com/bitcoin-core/secp256k1/
gcc -O2 -std=c99 -fPIC -I /usr/include/python2.7 -I secp256k1/trunk -I secp256k1/trunk/src -c secp256k1_libsecp256k1.c -o secp256k1_libsecp256k1.o
ld -shared -lcrypto -lgmp -lpython2.7 secp256k1_libsecp256k1.o -o secp256k1_libsecp256k1.so
