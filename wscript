#!/usr/bin/env python

top = '.'

from waflib import Configure, Logs


subdirs = """
krad_ebml_tools/krad_gui
krad_ebml_tools/krad_display
""".split()

def configure(conf):
	conf.check_tool('gcc')
	conf.check_tool('gnu_dirs')
	conf.env.append_unique('CFLAGS', ['-g', '-Wall', '-Wno-unused-variable', '-Wno-unused-but-set-variable'])
	conf.recurse(subdirs)
	
def build(bld):
    bld.recurse(subdirs)
