#!/bin/bash

# --enable-debug breaks build, recompile failed file (libavcodec/cavsdsp_mmx.c)
# with -fomit-frame-pointer
# http://lists.mplayerhq.hu/pipermail/mplayer-dev-eng/2006-November/047633.html
# http://wiki.multimedia.cx/index.php?title=MPlayer_FAQ#Build_questions.2Fcrashes

MCROOT=/Users/aw/Projects/motionbox/encoder/rendermix/demo_codec_sdk_v7.5_release
./configure \
  --prefix=/Users/aw/Projects/software/encoding/installed \
  --codecsdir=/Users/aw/Projects/software/encoding/installed/codecs \
  --with-extralibdir=/Users/aw/Projects/software/encoding/installed/lib:/opt/local/lib \
  --with-extraincdir=/Users/aw/Projects/software/encoding/installed/include:/opt/local/include:"$MCROOT/include" \
  --extra-libs=-F"$MCROOT/Frameworks" \
  --disable-radio-v4l2 \
  --disable-radio-bsdbt848 \
  --disable-tv \
  --disable-tv-v4l1 \
  --disable-tv-v4l2 \
  --disable-tv-bsdbt848 \
  --disable-tv-teletext \
  --disable-pvr \
  --disable-unrarexec \
  --disable-ftp \
  --disable-ass \
  --disable-xss \
  --disable-bitmap-font \
  --disable-freetype \
  --disable-fontconfig \
  --enable-mch264dec \
  --enable-xmms \
  --enable-gif \
  --enable-png \
  --enable-jpeg \
  --enable-tremor-external \
  --enable-theora \
  --enable-faad-external \
  --disable-apple-remote \
  --enable-debug \
;
