#!/usr/bin/env python

top = '.'

from waflib import Configure, Logs
import os


toolsdir = "krad_ebml_tools"
subdirs = os.listdir('./' + toolsdir)

for s in subdirs:
	subdirs[subdirs.index(s)] = os.getcwd() + "/" + toolsdir + "/" + s
	
def configure(conf):
	conf.check_tool('gcc')
	conf.check_tool('gnu_dirs')
	conf.env.append_unique('CFLAGS', ['-g', '-Wall', '-Wno-unused-variable', '-Wno-unused-but-set-variable'])
	conf.recurse(subdirs, mandatory = False)
	
def build(bld):
    bld.recurse(subdirs, mandatory = False)
