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
        -linux (default)    -
        -freebsd            -
        -win32              -

    [byte order]: (defaults to platform's)
        -le                 - compile for little endian arch
        -be                 - compile for big endian arch

    [database backend]:
        -cassandra (default)-
	-pgsql              -
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
	#core
	"bitset.h",
	"buffer_util.h",
	"config.h",
	"def.h",
	"dispatcher.h",
	"endian.h",
	"heapstring.h",
	"log.h",
 	"ringbuffer.h",
	"scheduler.h",
	"scopeguard.h",
	"shared.h",
	"stackstring.h",
	"stringbase.h",
	"stringutil.h",
	"system.h",
	"throw.h",

	#crypto
	"crypto/adler32.h",
	"crypto/base64.h",
	"crypto/bcrypt.h",
	"crypto/blowfish.h",
	"crypto/random.h",
	"crypto/rsa.h",
	"crypto/xtea.h",

	#db
	"db/blob.h",
	"db/db.h",
	"db/local.h",

	#game
	"game/account.h",
	"game/protocol_game.h",
	"game/protocol_info.h",
	"game/protocol_legacy.h",
	"game/protocol_login.h",

	#server
	"server/message.h",
	"server/outputmessage.h",
	"server/protocol.h",
	"server/protocol_test.h",
	"server/server.h",
	"server/server_rsa.h",


	#shard
]

OBJECTS = [
	#core
	"config",
	"dispatcher",
	"heapstring",
	"log",
	"main",
	"scheduler",
	"stringbase",
	"stringutil",
	"system",

	#crypto
	"crypto/adler32",
	"crypto/base64",
	"crypto/bcrypt",
	"crypto/blowfish",
	"crypto/random",
	"crypto/rsa",
	"crypto/xtea",

	#db
	"db/blob",
	"db/cassandra",
	"db/local",
	"db/pgsql",

	#game
	"game/protocol_login",

	#server
	"server/message",
	"server/outputmessage",
	"server/protocol_test",
	"server/server_epoll",
	"server/server_iocp",
	"server/server_kqueue",
	"server/server_rsa",

	#shard
	"shard/shard",
]

if __name__ == "__main__":
	# parse parameters
	output		= "kaplar"
	srcdir		= "src/"
	builddir	= "build/"
	test		= False
	compiler	= "CLANG"
	build		= "RELEASE"
	platform	= "LINUX"
	database	= "CASSANDRA"

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
		elif opt == "-linux":
			platform = "LINUX"

		elif opt == "-freebsd":
			platform = "FREEBSD"

		elif opt == "-windows":
			platform = "WINDOWS"


		#endianess
		elif opt == "-le":
			byteorder = "LITTLE"

		elif opt == "-be":
			byteorder = "BIG"

		#database
		elif opt == "-cassandra":
			database = "CASSANDRA"

		elif opt == "-pgsql":
			database = "PGSQL"

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
	INCLUDES	= ["-I/usr/include/lua5.1"]
	DEFINES		= ["-D_XOPEN_SOURCE=700",
				"-D__BSD_VISIBLE=1"]
	LIBS		= ["-lstdc++", "-lpthread",
				"-lgmp", "-llua5.1"]

	#check compiler
	if compiler == "GCC":
		CXX = "g++"
	elif compiler == "CLANG":
		CXX = "clang++"
	else:
		print("[error] invalid compiler")
		sys.exit()

	#check platform
	if platform == "LINUX":
		DEFINES.append("-DPLATFORM_LINUX=1")
	elif platform == "FREEBSD":
		DEFINES.append("-DPLATFORM_FREEBSD=1")
	elif platform == "WINDOWS":
		DEFINES.append("-DPLATFORM_WINDOWS=1")
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
		LFLAGS.append("-Og")
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

	#check database backend
	if database == "CASSANDRA":
		DEFINES.append("-D__DB_CASSANDRA__")
	elif database == "PGSQL":
		DEFINES.append("-D__DB_PGSQL__")
		LIBS.append("-lpq")
	else:
		print("[error] invalid database backend")
		sys.exit()

	#concat CXXFLAGS, INCLUDES and DEFINES
	CXXFLAGS.extend(INCLUDES)
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

