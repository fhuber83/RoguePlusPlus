#!/usr/bin/env python3
"""Group the leading includes of each file given, as the project lays them
out: the file's own header (or a test's ../support fixture), <gtest/gtest.h>,
the standard headers, other <...> headers, then the project's "..." headers,
each group sorted and set off by a blank line. An include with a comment
after it (<curses.h>) stays last, where it is, but for an IWYU pragma, which
stays with its include."""
import os, re, sys

for path in sys.argv[1:]:
    lines = open(path).read().split('\n')
    # the leading region: comments before it stay, it ends at the first line
    # that is neither an include nor blank
    start = next((i for i, l in enumerate(lines) if l.startswith('#include')), None)
    if start is None:
        continue
    end = start
    while end < len(lines) and (lines[end].startswith('#include') or not lines[end].strip()):
        end += 1
    incs = [l for l in lines[start:end] if l.startswith('#include')]
    tail = [l for l in incs if re.search(r'[>"]\s*//', l) and 'IWYU pragma' not in l]
    incs = [l for l in incs if l not in tail]
    pragmas = {re.match(r'#include\s+([<"][^>"]+[>"])', l).group(1): l for l in incs if 'IWYU pragma' in l}
    names = sorted(set(re.match(r'#include\s+([<"][^>"]+[>"])', l).group(1) for l in incs))
    stem = re.sub(r'^(src|tests)/', '', path).rsplit('.', 1)[0]
    own = [n for n in names if n == f'"{stem}.hpp"' or n.startswith('"../support/')]
    rest = [n for n in names if n not in own]
    gtest = [n for n in rest if n == '<gtest/gtest.h>']
    std = [n for n in rest if n.startswith('<') and '/' not in n and '.' not in n]
    other = [n for n in rest if n.startswith('<') and n not in std + gtest]
    proj = [n for n in rest if n.startswith('"')]
    groups = [g for g in (own, gtest, std, other, proj, ) if g]
    block = []
    for g in groups:
        block += [pragmas.get(n, f'#include {n}') for n in g] + ['']
    if tail:
        block += tail + ['']
    new = lines[:start] + block + lines[end:]
    open(path, 'w').write('\n'.join(new))
