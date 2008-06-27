#!/bin/bash

# --enable-debug breaks build, recompile failed file (libavcodec/cavsdsp_mmx.c)
# with -fomit-frame-pointer
# http://lists.mplayerhq.hu/pipermail/mplayer-dev-eng/2006-November/047633.html
# http://wiki.multimedia.cx/index.php?title=MPlayer_FAQ#Build_questions.2Fcrashes

# Since we don't autodetect XviD, faac, libdca etc. we must explicitly include them in --extra-libs

MCROOT=/Users/aw/Projects/motionbox/encoder/rendermix/demo_codec_sdk_v7.5_release
./configure \
  --prefix=/Users/aw/Projects/software/encoding/installed \
  --codecsdir=/Users/aw/Projects/software/encoding/installed/codecs \
  --with-extralibdir=/Users/aw/Projects/software/encoding/installed/lib:/opt/local/lib \
  --with-extraincdir=/Users/aw/Projects/software/encoding/installed/include:/opt/local/include:"$MCROOT/include" \
  --extra-libs="-F$MCROOT/Frameworks -lxvidcore -lm -lfaac -ldts -lx264" \
  --disable-alsa \
  --disable-apple-ir \
  --disable-apple-remote \
  --disable-arts \
  --disable-ass \
  --disable-bitmap-font \
  --disable-caca \
  --disable-cddb \
  --disable-libcdio \
  --disable-cdparanoia \
  --disable-dvdnav \
  --disable-enca \
  --disable-esd \
  --disable-fontconfig \
  --disable-freetype \
  --disable-fribidi \
  --disable-ftp \
  --disable-jack \
  --disable-live \
  --disable-nas \
  --disable-network \
  --disable-openal \
  --disable-ossaudio \
  --disable-pulse \
  --disable-pvr \
  --disable-radio-bsdbt848 \
  --disable-radio-v4l2 \
  --disable-sgiaudio \
  --disable-smb \
  --disable-sortsub \
  --disable-sunaudio \
  --disable-toolame \
  --disable-tv \
  --disable-tv-bsdbt848 \
  --disable-tv-teletext \
  --disable-tv-v4l1 \
  --disable-tv-v4l2 \
  --disable-unrarexec \
  --disable-vidix \
  --disable-win32waveout \
  --disable-x11 \
  --disable-xmms \
  --disable-xss \
  --enable-faac \
  --enable-faad-external \
  --enable-gif \
  --enable-jpeg \
  --enable-largefiles \
  --enable-libamr_nb \
  --enable-libamr_wb \
  --enable-libdca \
  --enable-libdv \
  --enable-liblzo \
  --enable-mch264dec \
  --enable-musepack \
  --enable-png \
  --enable-pthreads \
  --enable-speex \
  --enable-theora \
  --enable-tremor-external \
  --enable-twolame \
  --enable-x264 \
  --enable-xvid \
;
#  --enable-yuv4mpeg \
#  --enable-debug \
