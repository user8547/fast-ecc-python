#!/usr/bin/python
import gmpy

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
        # curve parameters for secp256k1
        # http://perso.univ-rennes1.fr/sylvain.duquesne/master/standards/sec2_final.pdf
        self.a = 0
        self.b = 7
        self.p = 2**256-2**32-2**9-2**8-2**7-2**6-2**4-1
        gx = bi("79BE667E F9DCBBAC 55A06295 CE870B07 029BFCDB 2DCE28D9 59F2815B 16F81798".replace(" ", "").decode('hex'))
        gy = bi("483ADA77 26A3C465 5DA4FBFC 0E1108A8 FD17B448 A6855419 9C47D08F FB10D4B8".replace(" ", "").decode('hex'))
        self.g = [gx,gy]
        self.n = bi("FFFFFFFF FFFFFFFF FFFFFFFF FFFFFFFE BAAEDCE6 AF48A03B BFD25E8C D0364141".replace(" ", "").decode('hex'))

    def valid(self,point):
        xP = point[0]

        if xP==None:
            return False

        yP = point[1]
        return yP**2 % self.p == (pow(xP, 3, self.p) + self.a*xP + self.b) % self.p

    def decompress(self,compressed):
        byte = compressed[0]

        # point at infinity
        if byte=="\x00":
            return [None,None]

        xP = bi(compressed[1:])
        ysqr = (pow(xP, 3, self.p) + self.a*xP + self.b) % self.p
        assert self.p % 4 == 3
        yP = pow(ysqr, (self.p + 1)/4, self.p)
        assert pow(yP, 2, self.p)==ysqr
        if yP % 2:
            if byte=="\x03":
                return [xP,yP]
            return [xP, -yP % self.p]
        if byte=="\x02":
            return [xP,yP]
        return [xP, -yP % self.p]

    def compress(self,P):

        if P[0] == None:
            return "\x00" + "\x00"*32

        byte = "\x02"
        if P[1] % 2:
            byte = "\x03"
        return byte + ib(P[0])

    def inv(self,point):
        xP = point[0]

        if xP==None:
            return [None,None]

        yP = point[1]
        R = [xP,-yP % self.p]
        return R

    def add(self,P,Q):

        # P+P=2P
        if P==Q:
            return self.dbl(P)

        # P+0=P
        if P[0]==None:
            return Q
        if Q[0]==None:
            return P

        # P+-P=0
        if Q==self.inv(P):
            return [None,None]

        xP = P[0]
        yP = P[1]
        xQ = Q[0]
        yQ = Q[1]
        s = (yP - yQ) * gmpy.invert(xP - xQ, self.p) % self.p
        xR = (pow(s,2,self.p) - xP -xQ) % self.p
        yR = (-yP + s*(xP-xR)) % self.p
        R = [xR,yR]
        return R

    def dbl(self,P):
        # 2*0=0
        if P[0]==None:
            return P

        # yP==0
        if P[1]==0:
            return [None,None]

        xP = P[0]
        yP = P[1]
        s = (3*pow(xP,2,self.p)+self.a) * gmpy.invert(2*yP, self.p) % self.p
        xR = (pow(s,2,self.p) - 2*xP) % self.p
        yR = (-yP + s*(xP-xR)) % self.p
        R = [xR,yR]
        return R

    def mul(self, P, k):
        # x0=0
        if P[0]==None:
            return P

        N = P
        R = [None,None]

        while k:
            bit = k % 2
            k >>= 1
            if bit:
                R = self.add(R,N)
            N = self.dbl(N)

        return R

curve = Curve()
