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
CXX		= %s
CXXFLAGS	= %s
LFLAGS		= %s
LIBS		= %s

DEPS		=	\
	%s

kaplar:			\
	%s
	$(CXX) -o %s $^ $(LIBS) $(LFLAGS)

.PHONY: clean
clean:
	@rm -fR %s
'''

MakefileObject = r'''
%s: %s $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)
'''

DEPS = [
	"avltree.h", "cstring.h", "def.h", "dispatcher.h",
	"env.h", "log.h", "memblock.h", "ringbuffer.h",
	"scheduler.h", "system.h", "throw.h",

	#server core
	"server/asio.h",
	"server/connection.h",
	"server/message.h",
	"server/protocol.h",
	"server/server.h",

	#protocols
	"server/protocol_test.h",
]

COMMON = [
	"adler32", "dispatcher", "env", "log",
	"main", "scheduler", "system",

	#server core
	"server/connection",
	"server/message",
	"server/server",

	# protocols
	"server/protocol_test",
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
	CXX		= ""
	CXXFLAGS	= ["-std=c++14", "-Wall",
				"-Wno-pointer-sign",
				"-Wno-writable-strings"]
	LFLAGS		= [] 
	DEFINES		= ["-D_XOPEN_SOURCE=700"]
	LIBS		= ["-lstdc++", "-lpthread"]
	OBJECTS		= COMMON[:]

	#check compiler
	if compiler == "GCC":
		CXX = "g++"
	elif compiler == "CLANG":
		CXX = "clang++"
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
		DEFINES.append("-D__BSD_VISIBLE=1")
	else:
		print("[error] invalid platform")
		sys.exit()

	#check build
	if build == "RELEASE":
		LFLAGS.append("-s")
		LFLAGS.append("-O3")
	elif build == "DEBUG":
		CXXFLAGS.append("-g")
		LFLAGS.append("-g")
	else:
		print("[error] invalid build type")
		sys.exit()

	#check endianess
	if byteorder == "LITTLE":
		pass
	elif byteorder == "BIG":
		DEFINES.append("-D__BIG_ENDIAN__")
	else:
		print("[error] invalid byteorder")
		sys.exit()

	#concat CXXFLAGS and DEFINES
	CXXFLAGS.extend(DEFINES)

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
		file.write(MakefileHeader % (CXX,
			' '.join(CXXFLAGS),
			' '.join(LFLAGS),
			' '.join(LIBS),
			'\t\\\n\t'.join(DEPS),
			'\t\\\n\t'.join(list(zip(*OBJECTS))[0]),
			builddir + output, builddir))

		for obj in OBJECTS:
			file.write(MakefileObject % obj)

