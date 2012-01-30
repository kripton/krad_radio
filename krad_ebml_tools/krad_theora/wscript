#!/usr/bin/env python

programs = """
krad_theora_test.c
""".split()

sources = """
krad_theora.c
""".split()

depsources = """
""".split()

includedirs = """
""".split()

libs = ["theora", "theoraenc", "theoradec", "libswscale", "libavutil"]

def configure(conf):
	
	for l in libs:
		conf.check_cfg(package = l, uselib_store = l, args='--cflags --libs')


def build(bld):


	for p in programs:

		bld(features = 'c cprogram', 
			source = sources + depsources + [p], 
			includes = includedirs, 
			target = p.replace(".c", ""), 
			uselib = libs)

