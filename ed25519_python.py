#!/usr/bin/python
import sys

def bi(s):
    i = 0
    for c in s:
        i <<= 8
        i |= ord(c)
    return i

def ib(i,l=32):
    s = ""
    while l:
        s = chr(0xff & i) + s
        i >>= 8
        l -= 1
    return s

# curve implementation in python
class Curve:

    def __init__(self):
        # curve parameters for ed25519
        # http://ed25519.cr.yp.to/python/ed25519.py
        self.p = 2**255 - 19
        self.n = 2**252 + 27742317777372353535851937790883648493
        self.d = -121665 * pow(121666, self.p-2, self.p)
        self.I = pow(2,(self.p-1)/4,self.p)
        gy = 4*pow(5,self.p-2,self.p) % self.p
        gx = 15112221349535400772501151409588531511454012693041857206046113283949847762202
        #gy = 46316835694926478169428394003475163141307993866256225615783033603165251855960
        #gx = 15112221349535400772501151409588531511454012693041857206046113283949847762202
        self.g = [gx,gy]

    def valid(self,point):
        x = point[0]
        y = point[1]
        return (-x*x + y*y - 1 - self.d*x*x*y*y) % self.p == 0

    def decompress(self,compressed):
        byte = compressed[0]

        x = bi(compressed[1:])
        signbit = ord(byte) & 1

        # yrecover
        yy = (x*x+1) * pow(1-self.d*x*x, self.p-2, self.p)
        y = pow(yy,(self.p+3)/8,self.p)
        if (y*y - yy) % self.p != 0: y = (y*self.I) % self.p
        if y % 2 != 0: y = self.p-y

        if signbit != y & 1:
            y = self.p-y

        P = [x,y]
        if not self.valid(P):
            print "[-] decompress(): decoded point is not on curve!"
            print P
            sys.exit()
        return P


    def compress(self,P):
        byte = "\x02"
        x = P[0]
        y = P[1]
        if y & 1:
            byte = "\x03"
        return byte + ib(x)

    def inv(self,point):
        xP = point[0]
        yP = point[1]
        R = [-xP % self.p, yP]
        return R

    def add(self,P,Q):
        if P[0]==None:
            P = [0,1]
        x1 = P[0]
        y1 = P[1]
        x2 = Q[0]
        y2 = Q[1]
        x3 = (x1*y2+x2*y1) * pow(1+self.d*x1*x2*y1*y2, self.p-2, self.p)
        y3 = (y1*y2+x1*x2) * pow(1-self.d*x1*x2*y1*y2, self.p-2, self.p)
        R = [x3 % self.p, y3 % self.p]
        return R

    def mul(self, P, k):
        if k == 0: return [0,1]
        Q = self.mul(P,k/2)
        Q = self.add(Q,Q)
        if k & 1:
            Q = self.add(Q,P)

        if not self.valid(Q):
            print "[-] mul() result not on curve"
            sys.exit(0)

        return Q

curve = Curve()
