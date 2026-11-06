#!/usr/bin/env python3
# A small reader of Code::Blocks' Makefile.am files, so the Amiga build
# compiles the same sources and installs the same data as upstream's
# autotools build, without running configure for a host it doesn't know.
#   amake.py SRC CONF BUILD MK    writes the make rules for every library
#   amake.py --data SRC CONF SHARE  installs share/codeblocks (zips too)
# CONF lists the automake conditions that are true and the directories
# not to enter. Copyright (c) 2026 AmiCradle Team
import fnmatch, glob, os, re, shutil, sys, zipfile

def load_conf(path):
    true, skip, extra = set(), set(), {}
    for line in open(path):
        line = line.split('#', 1)[0].strip()
        if not line:
            continue
        word, _, rest = line.partition(' ')
        if word == 'true':
            true.update(rest.split())
        elif word == 'skip':
            skip.update(rest.split())
        elif word == 'flags':
            d, _, f = rest.partition(' ')
            extra[d] = f.strip()
    return true, skip, extra

class Am:
    """One Makefile.am: its variables after the conditions, and its rules."""
    def __init__(self, path, true):
        self.dir = os.path.dirname(path)
        self.vars, self.rules = {}, {}
        text = open(path).read().replace('\\\n', ' ')
        stack, rule = [], None
        for line in text.split('\n'):
            if line.startswith('\t') and rule is not None:
                if all(stack):
                    self.rules[rule].append(line.strip())
                continue
            rule = None
            s = line.split('#', 1)[0].strip() if not line.lstrip().startswith('#') else ''
            if not s:
                continue
            m = re.match(r'if\s+(!?)\s*(\w+)$', s)
            if m:
                v = m.group(2) in true
                stack.append(not v if m.group(1) else v)
                continue
            if s == 'else':
                stack[-1] = not stack[-1]
                continue
            if s == 'endif':
                stack.pop()
                continue
            if not all(stack):
                continue
            m = re.match(r'([\w.@$()-]+)\s*(\+?=)\s*(.*)$', s)
            if m and ':' not in m.group(1):
                k, op, v = m.groups()
                self.vars[k] = (self.vars.get(k, '') + ' ' + v) if op == '+=' else v
                continue
            m = re.match(r'([^:=\s]+)\s*:(?!=)', s)
            if m:
                rule = m.group(1)
                self.rules[rule] = []

    def expand(self, s, top, depth=0):
        if depth > 20:
            return s
        def sub(m):
            k = m.group(1)
            if k == 'srcdir':
                return self.dir
            if k == 'top_srcdir':
                return top
            if k == 'pkgdatadir' and k not in self.vars:
                return '$(datadir)/codeblocks'
            if k in ('datadir', 'WX_CXXFLAGS', 'WX_CFLAGS', 'PWD'):
                return '$(' + k + ')'
            if k == 'CB_SQUIRREL_CFLAGS':
                return '-I' + top + '/src/include/scripting/include'
            if k == 'CB_TINYXML_CFLAGS':
                return '-I' + top + '/src/include/tinyxml -DTIXML_USE_STL=YES'
            return self.expand(self.vars.get(k, ''), top, depth + 1)
        s = s.replace('@PACKAGE@', 'codeblocks')
        return re.sub(r'\$\((\w+)\)', sub, s)

def walk(top, true, skip):
    """Every Makefile.am under src/ that the build enters, in order."""
    out, todo = [], [os.path.join(top, 'src')]
    while todo:
        d = todo.pop(0)
        rel = os.path.relpath(d, top)
        if rel in skip or not os.path.isfile(os.path.join(d, 'Makefile.am')):
            continue
        am = Am(os.path.join(d, 'Makefile.am'), true)
        out.append(am)
        for s in am.expand(am.vars.get('SUBDIRS', ''), top).split():
            todo.append(os.path.normpath(os.path.join(d, s)))
    return out

def canon(name):
    return re.sub(r'[^A-Za-z0-9@]', '_', name)

def targets(am, top):
    """(kind, archive base, sources, cppflags) of each library or program."""
    for k, v in am.vars.items():
        m = re.match(r'(\w+?)_(LTLIBRARIES|LIBRARIES|PROGRAMS)$', k)
        if not m or m.group(1).startswith('EXTRA') or m.group(1) == 'check':
            continue
        for name in am.expand(v, top).split():
            c = canon(name)
            srcs = [w for w in am.expand(am.vars.get(c + '_SOURCES', ''), top).split()
                    if w.endswith(('.c', '.cpp', '.cxx', '.cc'))]
            cpp = am.vars.get('AM_CPPFLAGS', '') + ' ' + am.vars.get(c + '_CPPFLAGS', '')
            flags = (am.expand(cpp + ' ' + am.vars.get('AM_CFLAGS', '') + ' ' + am.vars.get(c + '_CFLAGS', ''), top),
                     am.expand(cpp + ' ' + am.vars.get('AM_CXXFLAGS', '') + ' ' + am.vars.get(c + '_CXXFLAGS', ''), top))
            base = re.sub(r'\.(la|a)$', '', name)
            kind = 'program' if m.group(2) == 'PROGRAMS' else ('plugin' if m.group(1) == 'pluginlib' else 'lib')
            if m.group(1) == 'noinst' and m.group(2) == 'PROGRAMS':
                continue
            yield kind, base, [os.path.join(am.dir, s) for s in srcs], flags

def makefile(top, conf, build, mk):
    true, skip, extra = load_conf(conf)
    rules, libs, plugins, progs = [], [], [], []
    for am in walk(top, true, skip):
        rel = os.path.relpath(am.dir, top)
        for kind, base, srcs, flags in targets(am, top):
            objs = []
            plug = ''
            if kind == 'plugin':
                plug = ' -DCB_STATIC_PLUGIN_FILE=\\"%s\\"' % re.sub(r'^lib', '', base)
            more = ' ' + extra.get(rel, '') + plug
            for s in srcs:
                o = 'obj/%s/%s.o' % (base, os.path.relpath(s, am.dir).replace('/', '_').rsplit('.', 1)[0])
                objs.append(o)
                cc = '$(CC) $(CBCFLAGS)' if s.endswith('.c') else '$(CXX) $(CBCXXFLAGS)'
                f = (flags[0] if s.endswith('.c') else flags[1]) + more
                rules.append('%s: %s\n\t@mkdir -p $(@D)\n\t%s %s -MMD -c %s -o $@\n' % (o, s, cc, f, s))
            if kind == 'program':
                progs.append((base, objs))
                continue
            a = 'lib/%s.a' % base
            (plugins if kind == 'plugin' else libs).append(a)
            rules.append('%s: Makefile %s\n\t@mkdir -p $(@D)\n\trm -f $@; $(AR) rcs $@ $(filter %%.o,$^)\n' % (a, ' '.join(objs)))
    with open(mk, 'w') as f:
        f.write('CB_LIBS = %s\nCB_PLUGINS = %s\n' % (' '.join(libs), ' '.join(plugins)))
        for base, objs in progs:
            f.write('%s_OBJS = %s\n' % (canon(base), ' '.join(objs)))
        f.write('\n'.join(rules))
        f.write('\n-include $(shell find obj -name "*.d" 2>/dev/null)\n')

def data(top, conf, share):
    """What make install puts under share/codeblocks, zips made from their rules."""
    true, skip, _ = load_conf(conf)
    n = 0
    for am in walk(top, true, skip):
        for k, v in am.vars.items():
            m = re.match(r'((?:dist_|nodist_|nobase_)*)(\w+?)_DATA$', k)
            if not m:
                continue
            nobase = 'nobase_' in m.group(1)
            d = am.vars.get(m.group(2) + 'dir', '$(pkgdatadir)' if m.group(2) == 'pkgdata' else '')
            dest = am.expand(d, top)
            if not dest.startswith('$(datadir)/codeblocks'):
                continue
            dest = os.path.join(share, dest[len('$(datadir)/'):])
            for w in am.expand(v, top).split():
                name = os.path.basename(w)
                if name.endswith('.zip') and (name in am.rules or w in am.rules):
                    os.makedirs(dest, exist_ok=True)
                    make_zip(am, top, am.rules.get(name) or am.rules[w], os.path.join(dest, name))
                    n += 1
                    continue
                p = w if os.path.isabs(w) else os.path.join(am.dir, w)
                for f in sorted(glob.glob(p)):
                    if not os.path.isfile(f):
                        continue
                    rel = os.path.relpath(f, am.dir) if nobase else os.path.basename(f)
                    os.makedirs(os.path.dirname(os.path.join(dest, rel)), exist_ok=True)
                    shutil.copyfile(f, os.path.join(dest, rel))
                    n += 1
    print('installed %d data files under %s' % (n, share))

def make_zip(am, top, recipe, out):
    line = am.expand(' '.join(recipe), top)
    m = re.search(r'cd\s+(\S+)\s*&&\s*zip\s+(.*?)(?:>|$)', line)
    if not m:
        raise SystemExit('no zip recipe for ' + out)
    cd, words = m.group(1), m.group(2).split()
    words = [w for w in words if not w.startswith('-') and not w.endswith('.zip')]
    if os.path.exists(out):
        os.remove(out)
    with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED) as z:
        seen = set()
        for w in words:
            for f in sorted(glob.glob(os.path.join(cd, w))):
                r = os.path.relpath(f, cd)
                if os.path.isfile(f) and r not in seen:
                    seen.add(r)
                    z.write(f, r)

if __name__ == '__main__':
    if sys.argv[1] == '--data':
        data(os.path.abspath(sys.argv[2]), sys.argv[3], sys.argv[4])
    else:
        makefile(os.path.abspath(sys.argv[1]), sys.argv[2], sys.argv[3], sys.argv[4])
