# This file handles debug developer builds.
# Certain files fail to compile with no optimization,
# these are special cased here.
# http://lists.mplayerhq.hu/pipermail/mplayer-dev-eng/2006-November/047633.html
# http://wiki.multimedia.cx/index.php?title=MPlayer_FAQ#Build_questions.2Fcrashes
#
# configure must be run with --enable-debug=gdb3, then this file must be
# included at end of the generated config.mak
#  include config-debug.mak

export MAKEFLAGS=-I ..

# --enable-debug still adds -O2, remove it
#MBFLAGS = $(filter-out -O2,$(CFLAGS))
CFLAGS := $(filter-out -O2,$(CFLAGS))
# ffmpeg subdirs use OPTFLAGS not CFLAGS
OPTFLAGS := $(filter-out -O2,$(CFLAGS))

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
