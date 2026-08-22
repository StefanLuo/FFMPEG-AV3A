.PHONY: encoder decoder install clean distclean
all clean distclean:
	$(MAKE) $@ -C libavs3_common
	$(MAKE) $@ -C avs2p3Decoder
	$(MAKE) $@ -C avs3Decoder
	$(MAKE) $@ -C avs3RM0Decoder
	$(MAKE) $@ -C avs2p3Encoder
	$(MAKE) $@ -C avs3Encoder
	$(MAKE) $@ -C avs3RM0Encoder
decoder:
	$(MAKE) -C libavs3_common
	$(MAKE) -C avs2p3Decoder
	$(MAKE) -C avs3Decoder
	$(MAKE) -C avs3RM0Decoder
encoder:
	$(MAKE) -C libavs3_common
	$(MAKE) -C avs2p3Encoder
	$(MAKE) -C avs3Encoder
	$(MAKE) -C avs3RM0Encoder
install: all
	cp -rf libavs3_common/libavs3_common.a libavs3_common/libavs3_common.so ../lib/
	cp -rf avs2p3Decoder/libavs2p3Decoder.a avs2p3Decoder/libavs2p3Decoder.so ../lib/
	cp -rf avs2p3Decoder/header ../include/avs2p3Decoder
	cp -rf avs3Decoder/libavs3Decoder.a avs3Decoder/libavs3Decoder.so ../lib/
	cp -rf avs3Decoder/include ../include/avs3Decoder
	cp -rf avs3RM0Decoder/avs3RM0Decoder ../bin/
	cp -rf avs2p3Encoder/libavs2p3Encoder.a avs2p3Encoder/libavs2p3Encoder.so ../lib/
	cp -rf avs2p3Encoder/include ../include/avs2p3Encoder
	cp -rf avs3Encoder/libavs3Encoder.a avs3Encoder/libavs3Encoder.so ../lib/
	cp -rf avs3Encoder/include ../include/avs3Encoder
	cp -rf avs3RM0Encoder/avs3RM0Encoder ../bin/

