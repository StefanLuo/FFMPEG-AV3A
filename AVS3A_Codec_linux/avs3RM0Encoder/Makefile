CFLAGS += -Wall -fPIC -g -std=c99 -I../avs3Encoder/include -I../avs2p3Encoder/include

LDFLAGS = -L../avs3Encoder -lavs3Encoder -L../avs2p3Encoder -lavs2p3Encoder -L../libavs3_common -lavs3_common -lm

C_SOURCES = $(wildcard *.c)

OBJS = $(subst .c,.o,$(C_SOURCES))

BIN = avs3RM0Encoder

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
