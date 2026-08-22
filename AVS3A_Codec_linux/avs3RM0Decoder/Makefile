CFLAGS += -Wall -fPIC -g -std=c99 -I../avs3Decoder/include -I../avs2p3Decoder/header

LDFLAGS = -L../avs3Decoder -lavs3Decoder -L../avs2p3Decoder -lavs2p3Decoder -L../libavs3_common -lavs3_common -lm

C_SOURCES = $(wildcard *.c)

OBJS = $(subst .c,.o,$(C_SOURCES))

BIN = avs3RM0Decoder

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $^ $(CFLAGS) $(LDFLAGS) -o $@


-include depends
depends: $(C_SOURCES)
	$(CC) -M $(CFLAGS) $^ > $@

install: all
	@echo nothing to install

clean:
	@rm -f $(OBJS) .*.dep $(BIN)
