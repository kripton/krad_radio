#!/usr/bin/env python

top = '.'
out = '.waf_build_directory'

from waflib.Errors import ConfigurationError
from waflib import Configure, Logs
import os, sys

toolsdir = "tools"
appsdir = "apps"
subdirs = os.listdir('./' + toolsdir)

for s in subdirs:
	subdirs[subdirs.index(s)] = os.getcwd() + "/" + toolsdir + "/" + s
	
subdirs += [os.getcwd() + "/" + appsdir]
	
def options(opt):

	opt.load('compiler_c')
	opt.load('compiler_cxx')
	
def check_way(way):
	try:  
	   os.environ["WAYRAD"]
	except KeyError: 
		return
	if os.environ['WAYRAD']:
		print("WAYRAD DETECTED!")
		way.env['WAYRAD'] = "yes"
		way.env.append_unique('CFLAGS', ['-DWAYRAD'])

def configure(conf):

	platform = sys.platform
	conf.env['IS_MACOSX'] = platform == 'darwin'
	conf.env['IS_LINUX'] = platform == 'linux' or platform == 'linux2'
    

	if conf.env['IS_LINUX']:
		print("Linux detected :D")
		conf.env.append_unique('CFLAGS', ['-DIS_LINUX'])
		conf.env.append_unique('CXXFLAGS', ['-DIS_LINUX'])

	if conf.env['IS_MACOSX']:
		print("MacOS X detected :(")
		conf.env.append_unique('CFLAGS', ['-DIS_MACOSX'])
		conf.env.append_unique('CXXFLAGS', ['-DIS_MACOSX'])

	check_way(conf)

	conf.load('compiler_c')	
	conf.load('compiler_cxx')

	conf.check_tool('gcc')
	conf.check_tool('gnu_dirs')
#	conf.env.append_unique('CFLAGS', ['-g', '-Wall', '-Wno-unused-variable', '-Wno-unused-but-set-variable'])
	conf.env.append_unique('CXXFLAGS', ['-g', '-Wall', '-Wno-write-strings'])
	conf.env.append_unique('CFLAGS', ['-g', '-Wall'])
#	conf.env.append_value("LINKFLAGS", ['-lrt','-ldl','-lm'])	
	conf.recurse(subdirs, mandatory = False)
	
def build(bld):
	check_way(bld)
	bld.recurse(subdirs, mandatory = False)
