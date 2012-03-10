#!/usr/bin/env python

top = '.'

from waflib.Errors import ConfigurationError
from waflib import Configure, Logs
import os, sys

klibdir = "krad_ebml"
toolsdir = "krad_tools"
appsdir = "krad_apps"
subdirs = os.listdir('./' + toolsdir)

for s in subdirs:
	subdirs[subdirs.index(s)] = os.getcwd() + "/" + toolsdir + "/" + s

subdirs += [os.getcwd() + "/" + klibdir]
	
subdirs += [os.getcwd() + "/" + appsdir]
	
def configure(conf):

	platform = sys.platform
	conf.env['IS_MACOSX'] = platform == 'darwin'
	conf.env['IS_LINUX'] = platform == 'linux' or platform == 'linux2'
    

	if conf.env['IS_LINUX']:
		print("Linux detected :D")

	if conf.env['IS_MACOSX']:
		print("MacOS X detected :(")

	conf.check_tool('gcc')
	conf.check_tool('gnu_dirs')
	conf.env.append_unique('CFLAGS', ['-g', '-Wall', '-Wno-unused-variable', '-Wno-unused-but-set-variable'])
	conf.recurse(subdirs, mandatory = False)
	
	print("\nHello to you " + os.getlogin() + "! Much thanks for testing this software, do enjoy!\n")
	
def build(bld):
    bld.recurse(subdirs, mandatory = False)
