#!/usr/bin/env python3
# Copyright (c) 2015–2018 Lars-Dominik Braun <lars@6xq.net>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

import re, sys, os
from bitarray import bitarray

# plain, coded, invert rd, invert value if rd negative
fivebsixbtbl = [
    (0, 0x06, True, True),
    (1, 0x11, True, True),
    (2, 0x12, True, True),
    (3, 0x23, False, False),
    (4, 0x14, True, True),
    (5, 0x25, False, False),
    (6, 0x26, False, False),
    (7, 0x38, False, True),
    (8, 0x18, True, True),
    (9, 0x29, False, False),
    (10, 0x2a, False, False),
    (11, 0x0b, False, False),
    (12, 0x2c, False, False),
    (13, 0x0d, False, False),
    (14, 0x0e, False, False),
    (15, 0x05, True, True),
    (16, 0x09, True, True),
    (17, 0x31, False, False),
    (18, 0x32, False, False),
    (19, 0x13, False, False),
    (20, 0x34, False, False),
    (21, 0x15, False, False),
    (22, 0x16, False, False),
    (23, 0x28, True, True),
    (24, 0x0c, True, True),
    (25, 0x19, False, False),
    (26, 0x1a, False, False),
    (27, 0x24, True, True),
    (28, 0x1c, False, False),
    (29, 0x22, True, True),
    (30, 0x21, True, True),
    (31, 0x0a, True, True),
    ]

# decoding table
fivebsixbdectbl = {}
for i, v, invertrd, invertval in fivebsixbtbl:
    assert v not in fivebsixbdectbl
    fivebsixbdectbl[v] = i
    if invertval:
        assert ~v&0x3f not in fivebsixbdectbl
        fivebsixbdectbl[(~v)&0x3f] = i

threebfourbtbl = [
    (0, 0x02, True, True),
    (1, 0x09, False, False),
    (2, 0x0a, False, False),
    (3, 0x0c, False, True),
    (4, 0x04, True, True),
    (5, 0x05, False, False),
    (6, 0x06, False, False),
    (7, 0x08, True, True),
    ]

# decoding table
threebfourbdectbl = {}
for i, v, invertrd, invertval in threebfourbtbl:
    assert v not in threebfourbdectbl
    threebfourbdectbl[v] = i
    if invertval:
        assert ~v&0xf not in threebfourbdectbl
        threebfourbdectbl[(~v)&0xf] = i

class CodeViolation (Exception):
    pass

class Truncated (Exception):
    pass

class UnusedBytes (Exception):
    pass

def decode (data, size):
    """
    Decode 8b10b encoded data with up to size bits.

    :return: (number of consumed bits, decoded bytes)
    """
    if not isinstance (data, bitarray):
        bits = bitarray (endian='little')
        bits.frombytes (data)
    if len (bits) < size:
        raise Truncated ('data is smaller than expected')
    i = 0
    res = []
    append = res.append
    try:
        while i+10 <= size:
            sixbits = bits[i:i+6].tobytes ()[0]
            low = fivebsixbdectbl[sixbits]
            i += 6
            fourbits = bits[i:i+4].tobytes ()[0]
            high = threebfourbdectbl[fourbits]
            i += 4
            append (high << 5 | low)
    except KeyError:
        raise CodeViolation (i)
    return i, bytes (res)

def genCTables ():
    # header
    invertdatabit = 7
    invertrdbit = 6

    print('/* 8b10b encoding/decoding tables, generated by {} */\n'.format (os.path.basename (sys.argv[0])))

    print('static const unsigned int invertDataBit = {}, invertRdBit = {};\n'.format (invertdatabit, invertrdbit))

    # 5b6b encoding
    fivebsixbtbl.sort (key = lambda x: x[0])
    print("""/* EDCBA lookup table, indexed by EDCBA input (A is least significant bit),
     * outputs: invert value (1 bit, ~), invert rd (1 bit, N), iedcba (6 bits, for
     * positive RD, a is least significant bit)
     */""")
    print('static const uint8_t fivebsixbEncTbl[] = {')
    print('\t      /*      ~Niedcba */')
    for i, v, invertrd, invertval in fivebsixbtbl:
        composite = v | (int (invertrd) << invertrdbit) | (int (invertval) << invertdatabit)
        print('\t0x{:02x}, /* D{:2d}: {:08b} */'.format (composite, i, composite))
    print('\t};\n')

    # 3b4b encoding
    threebfourbtbl.sort (key = lambda x: x[0])
    print("""/* HGF lookup table, indexed by HGF input (F is least significant bit), see
     * above.
     */""")
    print('static const uint8_t threebfourbEncTbl[] = {')
    print('\t      /*       ~NXXjhgf */')
    for i, v, invertrd, invertval in threebfourbtbl:
        composite = v | (int (invertrd) << invertrdbit) | (int (invertval) << invertdatabit)
        print('\t0x{:02x}, /* Dx.{:d}: {:08b} */'.format (composite, i, composite))
    print('\t};\n')

    # 5b6b decoding
    print("""/* iedcba lookup table, values are decoded data */""")
    print('static const uint8_t fivebsixbDecTbl[] = {')
    for i in range (64):
        if i in fivebsixbdectbl:
            print('\t0x{0:02x}, /* D{0:2d} */'.format (fivebsixbdectbl[i]))
        else:
            print('\t0xff, /* unused */')
    print('\t};\n')

    # 3b4b decoding
    print("""/* jhgf lookup table, values are decoded data */""")
    print('static const uint8_t threebfourbDecTbl[] = {')
    for i in range (16):
        if i in threebfourbdectbl:
            print('\t0x{0:02x}, /* Dx.{0:2d} */'.format (threebfourbdectbl[i]))
        else:
            print('\t0xff, /* unused */')
    print('\t};\n')

if __name__ == '__main__':
    genCTables ()

