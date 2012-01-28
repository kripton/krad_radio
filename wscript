#! /usr/bin/env python

top = '.'

from waflib import Configure, Logs
#Configure.autoconfig = True

#def options(opt):
#	opt.load('compiler_c')
#	opt.load('gnu_dirs')

def configure(conf):
	pass

subdirs = "krad_ebml_tools/krad_gui"

def build(bld):
    bld.recurse(subdirs)
