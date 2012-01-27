# Output binary to be built
TARGET=curl-loader
TAGFILE=.tagfile

BUILD=$(shell pwd)/build

#
# Building of DNS asynch resolving c-ares library.
#
CARES_BUILD=$(BUILD)/c-ares
CARES_VER:=1.7.5
CARES_MAKE_DIR=$(CARES_BUILD)/c-ares-$(CARES_VER)

CURL_BUILD=$(BUILD)/curl
CURL_VER:=7.24.0-20120109

LIBEVENT_BUILD=$(BUILD)/libevent
LIBEVENT_VER:=1.4.14b
LIBEVENT_MAKE_DIR=$(LIBEVENT_BUILD)/libevent-$(LIBEVENT_VER)-stable

OBJ_DIR:=obj
SRC_SUFFIX:=c
OBJ:=$(patsubst %.$(SRC_SUFFIX), $(OBJ_DIR)/$(basename %).o, $(wildcard *.$(SRC_SUFFIX)))

OPENSSLDIR=$(shell $(CURDIR)/openssldir.sh)

# C compiler
CC=gcc

#C Compiler Flags
CFLAGS= -W -Wall -Wpointer-arith -pipe \
	-DCURL_LOADER_FD_SETSIZE=20000 \
	-D_FILE_OFFSET_BITS=64

#
# Making options: e.g. $make optimize=1 debug=0 profile=1 
#
debug ?= 1
optimize ?= 1
profile ?= 0

#Debug flags
ifeq ($(debug),1)
DEBUG_FLAGS+= -g
else
DEBUG_FLAGS=
ifeq ($(profile),0)
OPT_FLAGS+=-fomit-frame-pointer
endif
endif

#Optimization flags
ifeq ($(optimize),1)
OPT_FLAGS+= -O3 -ffast-math -finline-functions -funroll-all-loops \
	-finline-limit=1000 -mmmx -msse -foptimize-sibling-calls
else
OPT_FLAGS= -O0
endif

# CPU-tuning flags for Pentium-4 arch as an example.
#
#OPT_FLAGS+= -mtune=pentium4 -mcpu=pentium4

# CPU-tuning flags for Intel core-2 arch as an example. 
# Note, that it is supported only by gcc-4.3 and higher
#OPT_FLAGS+=  -mtune=core2 -march=core2

#Profiling flags
ifeq ($(profile),1)
PROF_FLAG=-pg
else
PROF_FLAG=
endif


#Linker mapping
LD=gcc

#Linker Flags
LDFLAGS=-L./lib -L$(OPENSSLDIR)/lib

# Link Libraries. In some cases, plese add -lidn, or -lldap
LIBS= -lcurl -levent -lz -lssl -lcrypto -lcares -ldl -lpthread -lnsl -lrt -lresolv

# Include directories
INCDIR=-I. -I./inc -I$(OPENSSLDIR)/include

# Targets
LIBCARES:=./lib/libcares.a
LIBCURL:=./lib/libcurl.a
LIBEVENT:=./lib/libevent.a

# documentation directory
DOCDIR=/usr/share/doc/curl-loader/

# manual page directory
MANDIR=/usr/share/man

all: $(TARGET)

$(TARGET): $(LIBCARES) $(LIBCURL) $(LIBEVENT)  $(CONF_OBJ) $(OBJ)
	$(LD) $(PROF_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) -o $@ $(OBJ) $(LDFLAGS) $(LIBS)

nobuildcurl: $(OBJ)
	$(LD) $(PROF_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) -o $(TARGET) $(OBJ) $(LIBS)

clean:
	rm -f $(OBJ_DIR)/*.o $(TARGET) core*

cleanall: clean
	rm -rf ./build ./packages/curl-$(CURL_VER) \
	./packages/curl ./inc ./lib ./bin $(TAGFILE) \
	./packages/c-ares-$(CARES_VER) \
	*.log *.txt *.ctx *~ ./conf-examples/*~

tags:
	etags --members -o $(TAGFILE) *.h *.c

install:
	mkdir -p $(DESTDIR)/usr/bin 
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	mkdir -p $(DESTDIR)$(MANDIR)/man5
	mkdir -p $(DESTDIR)$(DOCDIR)
	cp -f curl-loader $(DESTDIR)/usr/bin
	cp -f doc/curl-loader.1 $(DESTDIR)$(MANDIR)/man1/  
	cp -f doc/curl-loader-config.5 $(DESTDIR)$(MANDIR)/man5/
	cp -f doc/* $(DESTDIR)$(DOCDIR) 
	cp -rf conf-examples $(DESTDIR)$(DOCDIR)

$(LIBEVENT):
	mkdir -p $(LIBEVENT_BUILD)
	cd $(LIBEVENT_BUILD); tar zxfv ../../packages/libevent-$(LIBEVENT_VER)-stable.tar.gz;
	cd $(LIBEVENT_MAKE_DIR); patch -p1 < ../../../patches/libevent-nevent.patch; ./configure --prefix $(LIBEVENT_BUILD) \
		CFLAGS="$(PROF_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS)"
	make -C $(LIBEVENT_MAKE_DIR); make -C $(LIBEVENT_MAKE_DIR) install
	mkdir -p ./inc; mkdir -p ./lib
	cp -pf $(LIBEVENT_BUILD)/include/*.h ./inc
	cp -pf $(LIBEVENT_BUILD)/lib/libevent.a ./lib

$(LIBCARES):
	mkdir -p $(CARES_BUILD)
	cd $(CARES_BUILD); tar zxf ../../packages/c-ares-$(CARES_VER).tar.gz;
	cd $(CARES_MAKE_DIR); ./configure --prefix $(CARES_MAKE_DIR) \
		CFLAGS="$(PROF_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS)"
	make -C $(CARES_MAKE_DIR); make -C $(CARES_MAKE_DIR) install
	mkdir -p ./inc; mkdir -p ./lib
	cp -pf $(CARES_MAKE_DIR)/include/*.h ./inc
	cp -pf $(CARES_MAKE_DIR)/lib/libcares.*a ./lib


# To enable IPv6 change --disable-ipv6 to --enable-ipv6

$(LIBCURL):
	cd ./packages; tar jxfv curl-$(CURL_VER).tar.bz2; ln -sf curl-$(CURL_VER) curl; \
	patch -d curl -p1 < ../patches/curl-trace-info-error.patch
	mkdir -p $(CURL_BUILD);
	cd $(CURL_BUILD); ../../packages/curl/configure --prefix=$(CURL_BUILD) \
	--without-libidn \
	--without-libssh2 \
	--disable-ldap \
	--disable-ipv6 \
        --enable-thread \
        --with-random=/dev/urandom \
        --with-ssl=/usr/include/openssl \
        --enable-shared=no \
        --enable-ares=$(CARES_MAKE_DIR) \
        CFLAGS="$(PROF_FLAG) $(DEBUG_FLAGS) $(OPT_FLAGS) -DCURL_MAX_WRITE_SIZE=4096"
	make -C $(CURL_BUILD); make -C $(CURL_BUILD)/lib install; make -C $(CURL_BUILD)/include/curl install;
	mkdir -p ./inc; mkdir -p ./lib
	cp -a $(CURL_BUILD)/include/curl ./inc/curl
	cp -pf $(CURL_BUILD)/lib/libcurl.*a ./lib


# Files types rules
.SUFFIXES: .o .c .h

*.o: *.h

$(OBJ_DIR)/%.o: %.c
	$(CC) $(CFLAGS) $(PROF_FLAG) $(OPT_FLAGS) $(DEBUG_FLAGS) $(INCDIR) -c -o $(OBJ_DIR)/$*.o $<

