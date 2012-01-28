#! /usr/bin/env python

top = '.'

from waflib import Configure, Logs


subdirs = """
krad_ebml_tools/krad_gui
""".split()

def configure(conf):
	conf.check_tool('gcc')
	conf.check_tool('gnu_dirs')
	
def build(bld):
    bld.recurse(subdirs)
