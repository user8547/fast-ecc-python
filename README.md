Fast elliptic curve point operations in Python 
==================

Python bindings for general purpose elliptic curve point operations.

Supported curves and implementations:
* secp256r1 (P-256/prime256v1) (OpenSSL)
* secp256r1 (P-256/prime256v1) (Python)
* secp256k1 (OpenSSL)
* secp256k1 (Python)
* secp256k1 ([libsecp256k1](https://github.com/bitcoin-core/secp256k1))
* Ed25519 ([ed25519-donna](https://github.com/floodyberry/ed25519-donna))
* Ed25519 (Python)

Example usage:
```
$ sh secp256k1_openssl.sh
$ python
>>> import secp256r1_openssl as curve
>>> curve.g
[48439561293906451759052585252797914202762949526041747995844080717082404635286L, 36134250956749795798585127919587881956611106672985015071877198253568414405109L]
>>> curve.mul(curve.g, 5)
[36794669340896883012101473439538929759152396476648692591795318194054580155373L, 101659946828913883886577915207667153874746613498030835602133042203824767462820L]
>>> curve.add(curve.g, curve.inv(curve.g))
[None, None]
>>> 
>>> curve.compress(curve.g).encode('hex')
'036b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296'
>>> curve.decompress('036b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296'.decode('hex'))
[48439561293906451759052585252797914202762949526041747995844080717082404635286L, 36134250956749795798585127919587881956611106672985015071877198253568414405109L]
```

Available methods:
- `mul(point, scalar)` - returns point multiplied with a scalar
- `add(point, point)` - returns addition of two points
- `inv(point)` - returns inverse of point
- `valid(point)` - returns 1 if point on curve, 0 otherwise
- `comress(point)` - returns 33 bytes - sign of Y coordinate (0x02 or 0x03) and X coordinate (32 bytes)
- `decompress(bytestring33)` - returns unpacked point

Point at infinity is represented as value `[Null, Null]` or 33 zero bytes (in compressed form).

Ed25519 does not have point at infinity. Neutral point is `[0,1]`.
