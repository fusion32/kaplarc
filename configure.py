#!/usr/bin/python

import sys

Usage = r'''
    USAGE: python configure.py [options]

Options: (options in the same section are mutually exclusive)

    -o <file>               - write output to <file>
    -srcdir <dir>           - change source directory
    -builddir <dir>         - change build directory
    -test                   - compiles ./main.cpp instead of <srcdir>/main.cpp
                              (this is useful for unit testing)

    [compiler]:
        -clang (default)    -
        -gcc                -

    [build type]:
        -release (default)  -
        -debug              -

    [platform]:
        -win32 (default)    -
        -linux              -
        -bsd                -

    [byte order]: (defaults to platform's)
        -le                 - compile for little endian arch
        -be                 - compile for big endian arch
'''

MakefileHeader = r'''
CC	= %s
CFLAGS	= %s
LDFLAGS	= %s
LDLIBS	= %s

DEPS	=		\
	%s

kaplar:			\
	%s
	$(CC) -o %s $^ $(LDLIBS) $(LDFLAGS)

.PHONY: clean
clean:
	@ rm -fR %s
'''

MakefileObject = r'''
%s: %s $(DEPS)
	@mkdir -p $(@D)
	$(CC) -c -o $@ $< $(CFLAGS)
'''

DEPS = [
	"avltree.h", "common.h", "cstring.h", "def.h",
	"log.h", "memblock.h", "ringbuffer.h", "scheduler.h",
	"system.h", "work.h", "workgroup.h",
]

COMMON = [
	"log", "main", "scheduler", "system",
	"work",
]

WIN32 = []
LINUX = []
BSD = []

if __name__ == "__main__":
	# parse parameters
	output		= "kaplar"
	srcdir		= "src/"
	builddir	= "build/"
	test		= False
	compiler	= "CLANG"
	build		= "RELEASE"
	platform	= "WIN32"

	#default to this platform byteorder
	byteorder	= "LITTLE"
	if sys.byteorder == "big":
		byteorder = "BIG"

	args = iter(sys.argv[1:])
	for opt in args:
		#output name
		if opt == "-o":
			output = next(args)

		#source dir
		elif opt == "-srcdir":
			srcdir = next(args)

		#build dir
		elif opt == "-builddir":
			builddir = next(args)

		#enable unit testing
		elif opt == "-test":
			test = True

		#compilers
		elif opt == "-clang":
			compiler = "CLANG"

		elif opt == "-gcc":
			compiler = "GCC"

		#build types
		elif opt == "-release":
			build = "RELEASE"

		elif opt == "-debug":
			build = "DEBUG"

		#platforms
		elif opt == "-win32":
			platform = "WIN32"

		elif opt == "-linux":
			platform = "LINUX"

		elif opt == "-bsd":
			platform = "BSD"

		#endianess
		elif opt == "-le":
			byteorder = "LITTLE"

		elif opt == "-be":
			byteorder = "BIG"

		# invalid option
		else:
			print("[warning] Invalid option used: \"%s\"" % opt)
			print(Usage)
			sys.exit()

	# set parameters
	CC	= ""
	CFLAGS	= "-std=c++14 -Wall -Wno-pointer-sign"
	CDEFS	= "-D_XOPEN_SOURCE=700"
	LDFLAGS	= ""
	LDLIBS	= "-lc++ -lpthread"
	OBJECTS	= COMMON[:]

	#check compiler
	if compiler == "GCC":
		CC = "g++"
	elif compiler == "CLANG":
		CC = "clang++"
	else:
		print("[error] invalid compiler")
		sys.exit()

	#check platform
	if platform == "WIN32":
		OBJECTS.extend(WIN32)
	elif platform == "LINUX":
		OBJECTS.extend(LINUX)
	elif platform == "BSD":
		OBJECTS.extend(BSD)
		CDEFS += " -D__BSD_VISIBLE=1"
	else:
		print("[error] invalid platform")
		sys.exit()

	#check build
	if build == "RELEASE":
		LDFLAGS = "-s -O2"
	elif build == "DEBUG":
		CFLAGS = "-g " + CFLAGS
		LDFLAGS = "-g"
	else:
		print("[error] invalid build type")
		sys.exit()

	#check endianess
	if byteorder == "LITTLE":
		pass
	elif byteorder == "BIG":
		CDEFS += " -D__BIG_ENDIAN__"
	else:
		print("[error] invalid byteorder")
		sys.exit()

	#concat CFLAGS and CDEFS
	CFLAGS += " " + CDEFS

	#add path to dependencies
	DEPS	= [srcdir + dep for dep in DEPS]

	#create tuple (obj, src) for each object
	OBJECTS = [(builddir + "obj/" + obj + ".o", srcdir + obj + ".cpp")
				for obj in OBJECTS]

	#if testing, change <srcdir>/main.cpp to ./main.cpp
	if test == True:
		for (a, b) in OBJECTS:
			if ("main.o" in a) or ("main.cpp" in b):
				OBJECTS.remove((a, b))
				OBJECTS.append((a, "main.cpp"))
				break

	# output to file
	with open("Makefile", "w") as file:
		file.write(MakefileHeader % (CC, CFLAGS, LDFLAGS, LDLIBS,
			'\t\\\n\t'.join(DEPS),
			'\t\\\n\t'.join(list(zip(*OBJECTS))[0]),
			builddir + output, builddir))

		for obj in OBJECTS:
			file.write(MakefileObject % obj)

