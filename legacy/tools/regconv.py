#!/usr/bin/env python3
"""
LupoUsbDriver legacy port: convert StormC/SAS-C register parameters

    register __a0 PUSB_SUPPORT Req      ->  PUSB_SUPPORT Req __asm("a0")
    register __d0 UWORD                 ->  UWORD _d0 __asm("d0")   (unnamed)

to the syntax understood by m68k-amigaos-gcc. Applied once to legacy/src;
the result is committed, so the change is visible in the history.

usage: regconv.py FILE...
"""
import re
import sys

REG = re.compile(r'register\s+__([ad][0-7])\s+([^,()]*?)\s*(?=[,)])')
TYPE_WORDS = {'struct', 'union', 'enum', 'const', 'volatile', 'unsigned', 'signed'}


def convert(m):
    reg, decl = m.group(1), m.group(2).strip()
    # identifiers in the declaration, 'struct X' counted as one type name
    words = re.findall(r'[A-Za-z_]\w*', decl)
    ids = []
    skip_next = False
    for w in words:
        if skip_next:
            skip_next = False
            continue
        if w in ('struct', 'union', 'enum'):
            skip_next = True
            ids.append('<tag>')
            continue
        if w in TYPE_WORDS:
            continue
        ids.append(w)
    if len(ids) <= 1:                       # type only, no parameter name
        decl = decl + ' _' + reg
    return '%s __asm("%s")' % (decl, reg)


def main():
    total = 0
    for path in sys.argv[1:]:
        with open(path, 'rb') as f:
            text = f.read().decode('latin-1')
        new, n = REG.subn(convert, text)
        if n:
            with open(path, 'wb') as f:
                f.write(new.encode('latin-1'))
            print('%-50s %4d' % (path, n))
            total += n
    print('total', total)


if __name__ == '__main__':
    main()
