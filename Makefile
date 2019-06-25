
CXX		= clang++
CXXFLAGS	= -std=c++14 -Wall -Wno-pointer-sign -Wno-writable-strings -I/usr/include/lua5.1 -D_XOPEN_SOURCE=700 -D__BSD_VISIBLE=1 -DPLATFORM_LINUX=1 -D__DB_PGSQL__
LFLAGS		= -s -O3
LIBS		= -lstdc++ -lpthread -lgmp -llua5.1 -lpq

DEPS		=	\
	src/bitset.h	\
	src/buffer_util.h	\
	src/config.h	\
	src/def.h	\
	src/dispatcher.h	\
	src/endian.h	\
	src/heapstring.h	\
	src/log.h	\
	src/ringbuffer.h	\
	src/scheduler.h	\
	src/scopeguard.h	\
	src/shared.h	\
	src/stackstring.h	\
	src/stringbase.h	\
	src/stringutil.h	\
	src/system.h	\
	src/throw.h	\
	src/crypto/adler32.h	\
	src/crypto/base64.h	\
	src/crypto/bcrypt.h	\
	src/crypto/blowfish.h	\
	src/crypto/random.h	\
	src/crypto/rsa.h	\
	src/crypto/xtea.h	\
	src/db/blob.h	\
	src/db/db.h	\
	src/db/local.h	\
	src/game/account.h	\
	src/game/protocol_game.h	\
	src/game/protocol_info.h	\
	src/game/protocol_legacy.h	\
	src/game/protocol_login.h	\
	src/server/message.h	\
	src/server/outputmessage.h	\
	src/server/protocol.h	\
	src/server/protocol_test.h	\
	src/server/server.h	\
	src/server/server_rsa.h

kaplar:			\
	build/obj/config.o	\
	build/obj/dispatcher.o	\
	build/obj/heapstring.o	\
	build/obj/log.o	\
	build/obj/main.o	\
	build/obj/scheduler.o	\
	build/obj/stringbase.o	\
	build/obj/stringutil.o	\
	build/obj/system.o	\
	build/obj/crypto/adler32.o	\
	build/obj/crypto/base64.o	\
	build/obj/crypto/bcrypt.o	\
	build/obj/crypto/blowfish.o	\
	build/obj/crypto/random.o	\
	build/obj/crypto/rsa.o	\
	build/obj/crypto/xtea.o	\
	build/obj/db/blob.o	\
	build/obj/db/cassandra.o	\
	build/obj/db/local.o	\
	build/obj/db/pgsql.o	\
	build/obj/game/protocol_login.o	\
	build/obj/server/message.o	\
	build/obj/server/outputmessage.o	\
	build/obj/server/protocol_test.o	\
	build/obj/server/server_epoll.o	\
	build/obj/server/server_iocp.o	\
	build/obj/server/server_kqueue.o	\
	build/obj/server/server_rsa.o	\
	build/obj/shard/shard.o
	$(CXX) -o build/kaplar $^ $(LIBS) $(LFLAGS)

.PHONY: clean
clean:
	@rm -fR build/

build/obj/config.o: src/config.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/dispatcher.o: src/dispatcher.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/heapstring.o: src/heapstring.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/log.o: src/log.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/main.o: src/main.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/scheduler.o: src/scheduler.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/stringbase.o: src/stringbase.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/stringutil.o: src/stringutil.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/system.o: src/system.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/crypto/adler32.o: src/crypto/adler32.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/crypto/base64.o: src/crypto/base64.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/crypto/bcrypt.o: src/crypto/bcrypt.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/crypto/blowfish.o: src/crypto/blowfish.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/crypto/random.o: src/crypto/random.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/crypto/rsa.o: src/crypto/rsa.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/crypto/xtea.o: src/crypto/xtea.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/db/blob.o: src/db/blob.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/db/cassandra.o: src/db/cassandra.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/db/local.o: src/db/local.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/db/pgsql.o: src/db/pgsql.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/game/protocol_login.o: src/game/protocol_login.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/server/message.o: src/server/message.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/server/outputmessage.o: src/server/outputmessage.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/server/protocol_test.o: src/server/protocol_test.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/server/server_epoll.o: src/server/server_epoll.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/server/server_iocp.o: src/server/server_iocp.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/server/server_kqueue.o: src/server/server_kqueue.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/server/server_rsa.o: src/server/server_rsa.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)

build/obj/shard/shard.o: src/shard/shard.cpp $(DEPS)
	@mkdir -p $(@D)
	$(CXX) -c -o $@ $< $(CXXFLAGS)
