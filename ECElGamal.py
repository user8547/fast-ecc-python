#!/usr/bin/python

#import secp256r1_openssl as curve
from secp256r1_python import curve
#import secp256k1_openssl as curve
#from secp256k1_python import curve
#import secp256k1_libsecp256k1 as curve
#import ed25519_donna as curve
#from ed25519_python import curve

import random

def keygen():
    x = random.randint(1, curve.n-1)
    h = curve.mul(curve.g, x)
    return x, h

def encrypt(p, h):

    # encrypt point p
    r = random.randint(1, curve.n-1)
    c1 = curve.mul(curve.g, r)
    c2 = curve.add( curve.mul(h, r), p)
    return c1, c2

def decrypt(x, c1, c2):
    p = curve.add( curve.inv(curve.mul(c1, x)), c2)
    return p

x, h = keygen()
p = curve.mul(curve.g, 5)
print "point to be encrypted:", p

c1, c2 = encrypt(p, h)
print "ciphertext:", (curve.compress(c1)+curve.compress(c2)).encode('hex')
print "ciphertext c1:", c1
print "ciphertext c2:", c2

p_dec = decrypt(x, c1, c2)
print "decrypted point:", p_dec

