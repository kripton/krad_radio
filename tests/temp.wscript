#!/usr/bin/env python

programs1 = """
krad_dirac_test.c
""".split()

sources = """
krad_dirac.c
""".split()

depsources1 = """
""".split()

includedirs1 = """
""".split()


libs1 = ["orc-0.4", "schroedinger-1.0"]

def configure(conf):
	
	for l in libs1:
		conf.check_cfg(package = l, uselib_store = l, args='--cflags --libs')



def build(bld):


	for p in programs1:

		bld(features = 'c cprogram', 
			source = sources + depsources1 + [p], 
			includes = includedirs1, 
			target = p.replace(".c", ""), 
			uselib = libs1)
			
			
			
			
			
############


#!/usr/bin/env python

sources = """
krad_gui.c 
krad_gui_gtk.c
""".split()

includedirs = """
.
""".split()

libs = ["gtk+-3.0"]

def configure(conf):
	
	for l in libs:
		conf.check_cfg(package = l, uselib_store = l, args='--cflags --libs')
	
def build(bld):

	bld(features = 'c cprogram', 
		source = sources + ["krad_gui_gtk_test.c"], 
		includes = includedirs, 
		target = "krad_gui_gtk_test", 
		lib = ['m'],
		uselib = libs + ["m"])


#################


#!/usr/bin/env python

programs = """
krad_list_test.c
""".split()

sources = """
krad_list.c
""".split()

depsources = """
""".split()

includedirs = """
""".split()

libs = ["libxml-2.0"]

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
			
			
################


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


########################


#!/usr/bin/env python

programs = """
krad_v4l2_test.c
""".split()

sources = """
krad_v4l2.c
""".split()

depsources = """
""".split()

includedirs = """
/usr/local/include
/usr/include
""".split()

libs = ['turbojpeg']

def configure(conf):
	
	conf.check(header_name='linux/videodev2.h', features='c cprogram')
	
	for l in libs:
		conf.check_cfg(package = l, uselib_store = l, args='--cflags --libs')


def build(bld):


	for p in programs:

		bld(features = 'c cprogram', 
			source = sources + depsources + [p], 
			includes = includedirs, 
			target = p.replace(".c", ""), 
			uselib = libs)


############################

#!/usr/bin/env python

programs = """
krad_vpx_test.c
""".split()

sources = """
krad_vpx.c
""".split()

depsources = """
""".split()

includedirs = """
""".split()

libs = ["vpx"]

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


##########						
