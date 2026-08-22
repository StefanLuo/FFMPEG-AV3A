CFLAGS += -Wall -fPIC -g -std=c99 -I./header -I./libAFsp/Include -I./libAFsp/Include/libtsp

C_SOURCES := $(wildcard *.c) $(wildcard */*.c) $(wildcard */*/*.c)

C_SOURCES := $(filter-out general/decode/crc_16_temp.c, $(C_SOURCES))
C_SOURCES := $(filter-out general/decode/bitstream_temp.c, $(C_SOURCES))

OBJS = $(subst .c,.o,$(C_SOURCES))

SHARED = libavs2p3Decoder.so

STATIC = libavs2p3Decoder.a

.PHONY: all clean

all: $(SHARED) $(STATIC)

$(SHARED): $(OBJS)
	$(CC) -shared -o $@ $^

$(STATIC): $(OBJS)
	ar crus $@ $^
	ranlib $@


-include depends
depends: $(C_SOURCES)
	$(CC) -M $(CFLAGS) $^ > $@

install: all
	@echo nothing to install

clean:
	@rm -f $(OBJS) .*.dep $(SHARED) $(STATIC)
