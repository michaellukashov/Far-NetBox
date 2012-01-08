#!/usr/bin/python
# -*- coding: utf-8 -*-

import os, sys, string
import re, time

version_h = \
"""\
//------------------------------------------------------------------------------
// version.h
// Информация о версии программы
//
//------------------------------------------------------------------------------
#pragma once

#include <string>

#define V_MAJOR    %(version_major)s
#define V_MINOR    %(version_minor)s
#define V_PATCH    %(patch)s
#define V_REVISION %(revision)s

static const std::string CONST_VERSION_STR("%(version_major)s.%(version_minor)s.%(patch)s");
static const std::string CONST_REVISION_STR("%(revision)s");
static const std::string CONST_COMPILATION_TIME("%(compile_time)s");

"""

def write_header(version_major, version_minor, patch, revision):
    f = 'version.h'
    file_h = open(f, "w")
    compile_time = time.strftime('%d.%m.%Y %H:%M:%S')
    version_h_content = version_h % locals()
    file_h.write(version_h_content)
    close(file_h)
    return

def getstatusoutput(cmdstr):
    """ replacement for command.getstatusoutput """
    pipe = os.popen(cmdstr)
    text = pipe.read()
    sts = pipe.close()
    if sts is None: sts = 0
    return sts, text
    #--getstatusoutput

def get_git_revision():
    revision = ''
    git_re = re.compile(r'^.+g(\w+)$')
    git_re2 = re.compile(r'^(\w+)$')
    try:
        status, output = getstatusoutput('git describe --always')
        #print ('output = %s' % output)
        #print >>sys.stdout, status
        #print >>sys.stdout, output
        context = git_re.match(output)
        #print >>sys.stdout, context
        if context:
            git_revision = context.groups(1)[0]
            #print >>sys.stdout, git_revision
            revision = git_revision
        else:
            context = git_re2.match(output)
            if context:
                git_revision = context.groups(1)[0]
                revision = git_revision
    except Exception, info:
        revision = ''
        print >>sys.stderr, info
        pass
    return revision

def get_revision():
    revision = get_git_revision()
    if revision is None:
        revision = ''
    return revision

def main():
    ver_re = re.compile(r'^NetBox\s(\d+)\.(\d+)\.(\d+).*$')
    f = open('ChangeLog')

    for line in f.readlines():
        context = ver_re.match(line)
        if context:
            version_major = context.groups(1)[0]
            version_minor = context.groups(1)[1]
            patch = context.groups(1)[2]
            revision = get_revision()
            write_header(version_major, version_minor, patch, revision)
            break
    return

if __name__ == '__main__':
    main()
