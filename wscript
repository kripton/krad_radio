#!/usr/bin/env python

top = '.'
out = '.waf_build_directory'

from waflib.Errors import ConfigurationError
from waflib import Configure, Logs
import os, sys

libdir = "lib"
clientsdir = "clients"
daemondir = "daemon"
subdirs = os.listdir('./' + libdir)

for s in subdirs:
	subdirs[subdirs.index(s)] = os.getcwd() + "/" + libdir + "/" + s
	
subdirs += [os.getcwd() + "/" + clientsdir]
subdirs += [os.getcwd() + "/" + daemondir]

def options(opt):

	opt.load('compiler_c')
	opt.load('compiler_cxx')

	opt.add_option('--optimize', action='store_true', default=False,
		help='Compile with -O3 rather than -g')	

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

	conf.link_add_flags

	conf.env.append_unique('CFLAGS', ['-fPIC'])

	if conf.options.optimize == False:
		conf.env.append_unique('CXXFLAGS', ['-g', '-Wall', '-Wno-write-strings'])
		conf.env.append_unique('CFLAGS', ['-g', '-Wall'])
	else:
		conf.env.append_unique('CXXFLAGS', ['-O3', '-Wno-write-strings'])
		conf.env.append_unique('CFLAGS', ['-O3'])

	conf.recurse(subdirs, mandatory = False)
	
def build(bld):
	check_way(bld)
	bld.recurse(subdirs, mandatory = False)
	bld.add_post_fun(post)

def post(pst):
	if pst.cmd == 'install' and pst.env['IS_LINUX'] == True: 
		pst.exec_command('/sbin/ldconfig')
		print "Ran ldconfig"
