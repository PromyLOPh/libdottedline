"""
ceightbtenb testcases
"""

from ceightbtenb import *

res = decode (b'\x35\x02', 10)
assert res == b'\xff', res

res = decode (bytes ([0xca, 0x19, 0xad, 0x56, 0xa5]), 4*10)
assert res == bytes ([0xff, 0x00, 0xaa, 0x55]), res

try:
    res = decode (b'\x00\x00', 10)
except CodeViolation:
    pass
else:
    assert False

# encode/decode
orig = bytes ([0xff, 0x00, 0xaa, 0x55])
encoded = encode (orig)
print (encoded)
assert encoded == bytes ([0xca, 0x19, 0xad, 0x56, 0xa5]), encoded
decoded = decode (encoded, len (encoded)*8)
assert orig == decoded, (orig, encoded, decoded)

