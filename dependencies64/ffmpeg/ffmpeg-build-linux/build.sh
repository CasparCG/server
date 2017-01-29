#!/bin/sh

set -e
set -u

jflag=
jval=2

while getopts 'j:' OPTION
do
  case $OPTION in
  j)	jflag=1
        	jval="$OPTARG"
	        ;;
  ?)	printf "Usage: %s: [-j concurrency_level] (hint: your cores + 20%%)\n" $(basename $0) >&2
		exit 2
		;;
  esac
done
shift $(($OPTIND - 1))

if [ "$jflag" ]
then
  if [ "$jval" ]
  then
    printf "Option -j specified (%d)\n" $jval
  fi
fi

cd `dirname $0`
ENV_ROOT=`pwd`
. ./env.source

rm -rf "$BUILD_DIR" "$TARGET_DIR"
mkdir -p "$BUILD_DIR" "$TARGET_DIR"

# NOTE: this is a fetchurl parameter, nothing to do with the current script
#export TARGET_DIR_DIR="$BUILD_DIR"

echo "#### FFmpeg static build, by STVS SA ####"
cd $BUILD_DIR
../fetchurl "http://www.tortall.net/projects/yasm/releases/yasm-1.3.0.tar.gz"
../fetchurl "http://www.zlib.net/fossils/zlib-1.2.8.tar.gz"
../fetchurl "http://www.bzip.org/1.0.6/bzip2-1.0.6.tar.gz"
#../fetchurl "http://downloads.sf.net/project/libpng/libpng15/older-releases/1.5.14/libpng-1.5.14.tar.gz"
../fetchurl "http://downloads.xiph.org/releases/ogg/libogg-1.3.2.tar.gz"
../fetchurl "http://downloads.xiph.org/releases/vorbis/libvorbis-1.3.5.tar.gz"
../fetchurl "http://downloads.xiph.org/releases/theora/libtheora-1.1.1.tar.bz2"
../fetchurl "http://storage.googleapis.com/downloads.webmproject.org/releases/webm/libvpx-1.4.0.tar.bz2"
#../fetchurl "http://downloads.sourceforge.net/project/faac/faac-src/faac-1.28/faac-1.28.tar.bz2"
../fetchurl "ftp://ftp.videolan.org/pub/x264/snapshots/last_x264.tar.bz2"
../fetchurl "http://downloads.xvid.org/downloads/xvidcore-1.3.3.tar.gz"
../fetchurl "http://downloads.sourceforge.net/project/lame/lame/3.99/lame-3.99.5.tar.gz"
../fetchurl "http://downloads.xiph.org/releases/opus/opus-1.1.tar.gz"
../fetchurl "http://www.freedesktop.org/software/fontconfig/release/fontconfig-2.11.94.tar.bz2"
../fetchurl "http://www.ffmpeg.org/releases/ffmpeg-2.7.tar.bz2"
../fetchurl "http://downloads.sourceforge.net/project/expat/expat/2.1.0/expat-2.1.0.tar.gz"
../fetchurl "ftp://ftp.gnutls.org/gcrypt/gnutls/v3.3/gnutls-3.3.15.tar.xz"
../fetchurl "https://ftp.gnu.org/gnu/nettle/nettle-2.7.1.tar.gz"
../fetchurl "https://gmplib.org/download/gmp/gmp-6.0.0a.tar.xz"
../fetchurl "https://github.com/libass/libass/archive/0.12.2.tar.gz"
../fetchurl "http://fribidi.org/download/fribidi-0.19.6.tar.bz2"
../fetchurl "ftp://ftp.videolan.org/pub/videolan/libbluray/0.8.1/libbluray-0.8.1.tar.bz2"
../fetchurl "http://tukaani.org/xz/xz-5.2.1.tar.xz"
../fetchurl "http://pkgs.fedoraproject.org/lookaside/pkgs/libgme/game-music-emu-0.6.0.tar.bz2/b98fafb737bc889dc65e7a8b94bd1bf5/game-music-emu-0.6.0.tar.bz2"
../fetchurl "http://ffmpeg.zeranoe.com/builds/source/external_libraries/libilbc-20141214-git-ef04ebe.tar.xz"
../fetchurl "http://downloads.sourceforge.net/project/modplug-xmms/libmodplug/0.8.8.5/libmodplug-0.8.8.5.tar.gz?r=&ts=1440421934&use_mirror=vorboss"
../fetchurl "http://downloads.sourceforge.net/project/opencore-amr/opencore-amr/opencore-amr-0.1.3.tar.gz"
../fetchurl "http://rtmpdump.mplayerhq.hu/download/rtmpdump-2.3.tgz"
../fetchurl "https://www.openssl.org/source/old/1.0.2/openssl-1.0.2a.tar.gz"
../fetchurl "http://downloads.sourceforge.net/project/soxr/soxr-0.1.1-Source.tar.xz"
../fetchurl "https://github.com/georgmartius/vid.stab/archive/release-0.98.tar.gz"
../fetchurl "http://download.savannah.gnu.org/releases/freetype/freetype-2.5.5.tar.bz2"
../fetchurl "http://ffmpeg.zeranoe.com/builds/source/external_libraries/libgsm-1.0.13-4.tar.xz"
../fetchurl "http://ffmpeg.zeranoe.com/builds/source/external_libraries/openjpeg-1.5.2.tar.xz"
../fetchurl "http://pkgs.fedoraproject.org/lookaside/pkgs/orc/orc-0.4.18.tar.gz/1a2552e8d127526c48d644fe6437b377/orc-0.4.18.tar.gz"
../fetchurl "http://ftp.cs.stanford.edu/pub/exim/pcre/pcre-8.36.tar.bz2"
../fetchurl "http://78.108.103.11/MIRROR/ftp/png/src/history/libpng12/libpng-1.2.53.tar.bz2"
../fetchurl "http://ffmpeg.zeranoe.com/builds/source/external_libraries/schroedinger-1.0.11.tar.xz"
../fetchurl "http://ffmpeg.zeranoe.com/builds/source/external_libraries/speex-1.2rc2.tar.xz"
../fetchurl "http://ftp.gnu.org/gnu/libtasn1/libtasn1-4.5.tar.gz"
../fetchurl "http://ffmpeg.zeranoe.com/builds/source/external_libraries/twolame-0.3.13.tar.xz"
../fetchurl "http://www.freedesktop.org/software/vaapi/releases/libva/libva-1.5.1.tar.bz2"
../fetchurl "http://ffmpeg.zeranoe.com/builds/source/external_libraries/vo-aacenc-0.1.3.tar.xz"
../fetchurl "http://ffmpeg.zeranoe.com/builds/source/external_libraries/vo-amrwbenc-0.1.2.tar.xz"
../fetchurl "http://ffmpeg.zeranoe.com/builds/source/external_libraries/wavpack-4.75.0.tar.xz"
../fetchurl "http://ffmpeg.zeranoe.com/builds/source/external_libraries/libwebp-0.4.3.tar.xz"
../fetchurl "http://ffmpeg.zeranoe.com/builds/source/external_libraries/x265-1.7.tar.xz"
../fetchurl "ftp://xmlsoft.org/libxml2/libxml2-2.9.2.tar.gz"

echo "*** Building libxml2 ***"
cd $BUILD_DIR/libxml2*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install || echo "Make install failed, continuing"

echo "*** Building x265 ***"
cd $BUILD_DIR/x265*/source
cmake .
make -j $jval
make DESTDIR=$TARGET_DIR install

echo "*** Building libwebp ***"
cd $BUILD_DIR/libwebp*
./configure --prefix=$TARGET_DIR --enable-shared --enable-mmx
make -j $jval
make install

echo "*** Building wavpack ***"
cd $BUILD_DIR/wavpack*
./configure --prefix=$TARGET_DIR --enable-shared --enable-mmx
make -j $jval
make install

echo "*** Building vo-amrwbenc ***"
cd $BUILD_DIR/vo-amrwbenc*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building vo-aacenc ***"
cd $BUILD_DIR/vo-aacenc*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building libva ***"
cd $BUILD_DIR/libva*
./configure --prefix=$TARGET_DIR --enable-shared --enable-drm=no --enable-glx=no --enable-egl=no --enable-wayland=no
make -j $jval
make install

echo "*** Building twolame ***"
cd $BUILD_DIR/twolame*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building libtasn1 ***"
cd $BUILD_DIR/libtasn1*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building speex ***"
cd $BUILD_DIR/speex*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building orc ***"
cd $BUILD_DIR/orc*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building schroedinger ***"
cd $BUILD_DIR/schroedinger*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building libpng ***"
cd $BUILD_DIR/libpng*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building pcre ***"
cd $BUILD_DIR/pcre*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building openjpeg ***"
cd $BUILD_DIR/openjpeg*
./bootstrap.sh
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building freetype ***"
cd $BUILD_DIR/freetype*
./configure --prefix=$TARGET_DIR --enable-shared --with-harfbuzz=no
make -j $jval
make install

echo "*** Building libgsm ***"
cd $BUILD_DIR/libgsm*
sed -i "s#INSTALL_ROOT	=#INSTALL_ROOT	= $TARGET_DIR#g" $BUILD_DIR/libgsm*/Makefile
sed -i 's#NeedFunctionPrototypes=1#NeedFunctionPrototypes=1 -fPIC#g' $BUILD_DIR/libgsm*/Makefile
sed -i 's#(GSM_INSTALL_ROOT)/inc#(GSM_INSTALL_ROOT)/include#g' $BUILD_DIR/libgsm*/Makefile
make -j $jval
make -j lib/libgsm.so
make install prefix=$TARGET_DIR
cp lib/libgsm.so* $TARGET_DIR/lib/

echo "*** Building xavs ***"
cd $BUILD_DIR/
svn checkout http://svn.code.sf.net/p/xavs/code/trunk xavs-code
cd xavs-code
./configure --prefix=$TARGET_DIR --enable-shared --disable-asm
make -j $jval
make install

echo "*** Building vid.stab ***"
cd $BUILD_DIR/vid.stab*
cmake .
make -j $jval
make DESTDIR=$TARGET_DIR install

echo "*** Building soxr ***"
cd $BUILD_DIR/soxr*
cmake -DWITH_OPENMP=off .
make -j $jval
make DESTDIR=$TARGET_DIR install

echo "*** Building openssl ***"
cd $BUILD_DIR/openssl*
./config --prefix=$TARGET_DIR shared
make
make install

echo "*** Building rtmpdump ***"
cd $BUILD_DIR/rtmpdump*
sed -i 's#LIB_OPENSSL=-lssl -lcrypto#LIB_OPENSSL=-lssl -lcrypto -ldl#g' $BUILD_DIR/rtmpdump*/Makefile
make INC=-I$TARGET_DIR/include LDFLAGS=-L$TARGET_DIR/lib  -j $jval SYS=posix
make install prefix=$TARGET_DIR

echo "*** Building opencore-amr ***"
cd $BUILD_DIR/opencore-amr*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building libmodplug ***"
cd $BUILD_DIR/libmodplug*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building libilbc ***"
cd $BUILD_DIR/libilbc*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building game-music-emu ***"
cd $BUILD_DIR/game-music-emu*
cmake .
make -j $jval
make DESTDIR=$TARGET_DIR install

echo "*** Building xz ***"
cd $BUILD_DIR/xz*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building libbluray ***"
cd $BUILD_DIR/libbluray*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install
sed -i 's#Libs.private: -ldl -lxml2   -lfreetype#Libs.private: -ldl -lxml2   -lfreetype -llzma#g' $TARGET_DIR/lib/pkgconfig/libbluray.pc

echo "*** Building fribidi ***"
cd $BUILD_DIR/fribidi*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building libass ***"
cd $BUILD_DIR/libass*
./autogen.sh
./configure --prefix=$TARGET_DIR --enable-shared --disable-harfbuzz --disable-enca
make -j $jval
make install

echo "*** Building expat ***"
cd $BUILD_DIR/expat*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building fontconfig ***"
cd $BUILD_DIR/fontconfig*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building gmp ***"
cd $BUILD_DIR/gmp*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building nettle ***"
cd $BUILD_DIR/nettle*
sed -i 's#testsuite##g' Makefile.in
sed -i 's#examples##g' Makefile.in
./configure --prefix=$TARGET_DIR --enable-shared --enable-mini-gmp
make -j $jval
make install

echo "*** Building gnutls ***"
cd $BUILD_DIR/gnutls*
./configure --prefix=$TARGET_DIR --enable-shared --disable-doc --without-p11-kit --disable-libdane --disable-cxx
make -j $jval
make install

echo "*** Building yasm ***"
cd $BUILD_DIR/yasm*
./configure --prefix=$TARGET_DIR
make -j $jval
make install

echo "*** Building zlib ***"
cd $BUILD_DIR/zlib*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building bzip2 ***"
cd $BUILD_DIR/bzip2*
make -f Makefile-libbz2_so
make install PREFIX=$TARGET_DIR

#echo "*** Building libpng ***"
#cd $BUILD_DIR/libpng*
#./configure --prefix=$TARGET_DIR --enable-static --disable-shared
#make -j $jval
#make install

# Ogg before vorbis
echo "*** Building libogg ***"
cd $BUILD_DIR/libogg*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

# Vorbis before theora
echo "*** Building libvorbis ***"
cd $BUILD_DIR/libvorbis*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building libtheora ***"
cd $BUILD_DIR/libtheora*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building livpx ***"
cd $BUILD_DIR/libvpx*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

#echo "*** Building faac ***"
#cd $BUILD_DIR/faac*
#./configure --prefix=$TARGET_DIR --enable-static --disable-shared
# FIXME: gcc incompatibility, does not work with log()

#sed -i -e "s|^char \*strcasestr.*|//\0|" common/mp4v2/mpeg4ip.h
#make -j $jval
#make install

echo "*** Building x264 ***"
cd $BUILD_DIR/x264*
./configure --prefix=$TARGET_DIR --enable-shared --disable-opencl
make -j $jval
make install

echo "*** Building xvidcore ***"
cd "$BUILD_DIR/xvidcore/build/generic"
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install
#rm $TARGET_DIR/lib/libxvidcore.so.*

echo "*** Building lame ***"
cd $BUILD_DIR/lame*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

echo "*** Building opus ***"
cd $BUILD_DIR/opus*
./configure --prefix=$TARGET_DIR --enable-shared
make -j $jval
make install

rm -f $TARGET_DIR/lib/*.a
rm -f $TARGET_DIR/usr/local/lib/*.a

# FFMpeg
echo "*** Building FFmpeg ***"
cd $BUILD_DIR/ffmpeg*
CFLAGS="-I$TARGET_DIR/include -I$TARGET_DIR/usr/local/include" LDFLAGS="-L$TARGET_DIR/lib -L$TARGET_DIR/usr/local/lib -lm" PKG_CONFIG_PATH=$TARGET_DIR/lib/pkgconfig:$TARGET_DIR/usr/local/lib/pkgconfig ./configure \
	--prefix=${OUTPUT_DIR:-$TARGET_DIR} \
	--extra-cflags="-I$TARGET_DIR/include -fexceptions -fnon-call-exceptions -fPIC" \
	--extra-ldflags="-L$TARGET_DIR/lib -lm -llzma" \
	--disable-doc \
	--disable-ffplay \
	--disable-ffserver \
	--disable-stripping \
	--enable-shared \
	--enable-avisynth \
	--enable-bzlib \
	--enable-fontconfig \
	--enable-frei0r \
	--enable-gnutls \
	--enable-gpl \
	--enable-iconv \
	--enable-libass \
	--enable-libbluray \
	--enable-libfreetype \
	--enable-libgme \
	--enable-libgsm \
	--enable-libilbc \
	--enable-libmodplug \
	--enable-libmp3lame \
	--enable-libopencore-amrnb \
	--enable-libopencore-amrwb \
	--enable-libopenjpeg \
	--enable-libopus \
	--enable-librtmp \
	--enable-libschroedinger \
	--enable-libsoxr \
	--enable-libspeex \
	--enable-libtheora \
	--enable-libtwolame \
	--enable-libvidstab \
	--enable-libvo-aacenc \
	--enable-libvo-amrwbenc \
	--enable-libvorbis \
	--enable-libvpx \
	--enable-libwavpack \
	--enable-libwebp \
	--enable-libx264 \
	--enable-libx265 \
	--enable-libxavs \
	--enable-libxvid \
	--enable-pthreads \
	--enable-version3 \
	--enable-libv4l2 \
	--enable-zlib
make -j $jval && make install

#dirty hack
#replace shipped ffmpeg linux libs with compiled ones from here!
cd $BUILD_DIR
mv ../../bin/linux ../../bin/linux-shipped
cp -r ../target/lib ../../bin/linux
#some ffmpeg libs are build in a different shared target folder
cp ../target/usr/local/lib/* ../../bin/linux/
