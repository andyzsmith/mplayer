# This file handles debug developer builds.
# Certain files fail to compile with no optimization,
# these are special cased here.
#
# configure must be run as usual, then this file must be
# included at end of the generated config.mak
#  include config-debug.mak
#
# Run make like:
#  MAKEFLAGS='-I ..' make

MBFLAGS = -Wdisabled-optimization -Wno-pointer-sign -Wdeclaration-after-statement -std=gnu99 -I. -W -Wall -march=pentium-m -mtune=pentium-m -pipe -ggdb3  -mdynamic-no-pic -falign-loops=16 -shared-libgcc -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -DHAVE_CONFIG_H $(EXTRA_INC)
CFLAGS = $(MBFLAGS)
# ffmpeg subdirs use OPTFLAGS not CFLAGS
OPTFLAGS = $(MBFLAGS)

liba52/imdct.o: CFLAGS += -O
mp3lib/dct64_sse.o: CFLAGS += -O
# libavcodec
h264.o: CFLAGS += -O -fomit-frame-pointer
h264_parser.o: CFLAGS += -O
cabac.o: CFLAGS += -O
i386/cavsdsp_mmx.o: CFLAGS += -O -fomit-frame-pointer
i386/dsputil_mmx.o: CFLAGS += -O
i386/snowdsp_mmx.o: CFLAGS += -O -fomit-frame-pointer
# libpostproc
postprocess.o: CFLAGS += -O
