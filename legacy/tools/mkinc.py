#!/usr/bin/env python3
"""
LupoUsbDriver legacy port: case-insensitive include resolution.

The original sources were written on AmigaOS, whose file system ignores
case: they include the same header as <usb/HcDriverStructs.h>,
<usb/hcdriverstructs.h>, "Usb/..." and so on. Instead of rewriting every
#include, this tool scans all #include directives of legacy/src and builds a
mirror directory where each spelling that is not found as written becomes a
symbolic link to the real file, found case-insensitively in:

  1. legacy/compat              (overrides: StormC pragma files, library glue)
  2. the directory of the including file
  3. legacy/src  (also as "usb/...": the headers were installed as <usb/...>)
  4. the generated headers (inline/proto for pci.library and lupousb.library)
  5. the NDK and the user's SDK copy (third_party/sdk/Include)

usage: mkinc.py SRC COMPAT GEN OUT SYSDIRS [MORE_INCDIRS...]

SYSDIRS is a colon separated list of the compiler's own include directories
(libnix, NDK...): a spelling found there as written is left to the compiler.
"""
import os
import re
import sys

INC = re.compile(r'^\s*#\s*include\s*([<"])([^>"]+)[>"]', re.M)


def index_tree(root):
    """lowercase relative path -> real path"""
    idx = {}
    if not os.path.isdir(root):
        return idx
    for dirpath, _, files in os.walk(root):
        for f in files:
            full = os.path.join(dirpath, f)
            rel = os.path.relpath(full, root).replace(os.sep, '/')
            idx.setdefault(rel.lower(), full)
    return idx


def main():
    src, compat, gen, out = sys.argv[1:5]
    sysdirs = [d for d in sys.argv[5].split(':') if d]
    incdirs = sysdirs + sys.argv[6:]
    compat_idx = index_tree(compat)
    src_idx = index_tree(src)
    gen_idx = index_tree(gen)
    sys_idx = {}
    for d in incdirs:
        for k, v in index_tree(d).items():
            sys_idx.setdefault(k, v)

    links = {}
    unresolved = []
    for dirpath, _, files in os.walk(src):
        for f in files:
            if not re.search(r'\.(c|C|h)$', f):
                continue
            path = os.path.join(dirpath, f)
            text = open(path, 'rb').read().decode('latin-1')
            for kind, name in INC.findall(text):
                key = name.replace('\\', '/').lower()
                # the compiler's own directories are searched anyway
                exact_sys = any(os.path.exists(os.path.join(d, name)) for d in sysdirs)
                target = compat_idx.get(key) or compat_idx.get(os.path.basename(key))
                if not target and kind == '"' and os.path.exists(os.path.join(dirpath, name)):
                    continue                    # found as written next to the source
                if not target:
                    rel_dir = os.path.relpath(dirpath, src).replace(os.sep, '/')
                    rel_key = (rel_dir.lower() + '/' + key) if rel_dir != '.' else key
                    target = src_idx.get(rel_key)
                    # StormC also found quoted includes relative to the parent
                    # directories (the directory of the including .c file)
                    up = rel_dir
                    while not target and up not in ('.', ''):
                        up = os.path.dirname(up) or '.'
                        cand = (up.lower() + '/' + key) if up != '.' else key
                        target = src_idx.get(cand)
                if not target and key.startswith('usb/'):
                    target = src_idx.get(key[4:])
                if not target:
                    target = src_idx.get(key) or gen_idx.get(key)
                if not target and exact_sys:
                    continue                    # system header with the right case
                if not target:
                    target = sys_idx.get(key)
                if not target:
                    unresolved.append('%s: %s' % (path, name))
                    continue
                old = links.get(name)
                if old and old != target:
                    print('warning: %s maps to %s and %s' % (name, old, target))
                links[name] = target

    for name, target in sorted(links.items()):
        dst = os.path.join(out, name)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        if os.path.lexists(dst):
            os.unlink(dst)
        os.symlink(os.path.abspath(target), dst)
    print('mkinc: %d include spellings mapped into %s' % (len(links), out))
    for u in unresolved:
        print('mkinc: unresolved %s' % u)


if __name__ == '__main__':
    main()
