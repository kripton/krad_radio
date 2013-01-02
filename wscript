#!/usr/bin/env python

top = '.'
out = '.waf_build_directory'

from waflib.Errors import ConfigurationError
from waflib import Configure, Logs
import os, sys
from subprocess import check_output

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

	opt.add_option('--without-x11', action='store_true', default=False, dest='nox11', 
		help='Don\'t build anything depending on X11 (X11 capture)')
		
	opt.add_option('--without-wayland', action='store_true', default=False, dest='nowayland', 
		help='Don\'t build anything depending on Wayland')
		
	opt.add_option('--without-gtk', action='store_true', default=False, dest='nogtk',
		help='Don\'t build anything depending on gtk+ (GUI)')
		
	opt.add_option('--without-gif', action='store_true', default=False, dest='nogif',
		help='Don\'t build anything depending on giflib-5.x')

def check_way(way):
	if way.options.nowayland == False:
		way.env['KRAD_USE_WAYLAND'] = "yes"
		way.env.append_unique('CFLAGS', ['-DKRAD_USE_WAYLAND'])

def check_x11(x11):
	if x11.options.nox11 == False:
		x11.env['KRAD_USE_X11'] = "yes"
		x11.env.append_unique('CFLAGS', ['-DKRAD_USE_X11'])
		
def get_git_ver(conf):
	try:
		gitver = check_output(["git", "describe", "--tags"])
		flag = '-DGIT_VERSION="' + gitver.decode('utf8').strip() + '"'
		conf.env.append_unique('CFLAGS', [flag])
		conf.env.append_unique('CXXLAGS', [flag])
	except OSError as e:
		print("Unable to get git tag/commit hash")

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

	check_x11(conf)
	check_way(conf)
	get_git_ver(conf)

	conf.load('compiler_c')	
	conf.load('compiler_cxx')

	conf.link_add_flags

	conf.env.append_unique('CFLAGS', ['-fPIC'])

	if conf.options.optimize == False:
		conf.env.append_unique('CXXFLAGS', ['-g', '-Wall', '-Wno-write-strings'])
		conf.env.append_unique('CFLAGS', ['-g', '-Wall'])
	else:
		#conf.env.append_unique('CXXFLAGS', ['-O3', '-Wno-write-strings', '-march=core-avx-i', '-ffast-math', '-ftree-vectorizer-verbose=5'])
		#conf.env.append_unique('CFLAGS', ['-O3', '-march=core-avx-i', '-ffast-math', '-ftree-vectorizer-verbose=5'])
		conf.env.append_unique('CXXFLAGS', ['-O3', '-Wno-write-strings'])
		conf.env.append_unique('CFLAGS', ['-O3'])

	conf.recurse(subdirs, mandatory = False)
	
def build(bld):
	bld.recurse(subdirs, mandatory = False)
	bld.add_post_fun(post)

def post(pst):
	if pst.cmd == 'install' and pst.env['IS_LINUX'] == True: 
		pst.exec_command('/sbin/ldconfig')
