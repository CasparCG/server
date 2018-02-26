apt-get install -yq --no-install-recommends \
                autoconf \
                automake \
                cmake \
                curl \
                bzip2 \
                libexpat1-dev \
                g++ \
                gcc \
                git \
                gperf \
                libtool \
                make \
                nasm \
                perl \
                pkg-config \
                python \
                libssl-dev \
                yasm \
                zlib1g-dev \
                libjpeg-dev \
                libsndfile1-dev \
                libglu1-mesa-dev \
                libharfbuzz0b \
                libpangoft2-1.0-0 \
                libcairo2 \
                libv4l-0 \
                libraw1394-11 \
                libavc1394-0 \
                libiec61883-0 \
                libxtst6 \
                libnss3 \
                libnspr4 \
                libgconf-2-4 \
                libasound2 \
                libfreeimage-dev \
                libglew-dev \
                libopenal-dev \
                libopus-dev \
                libgsm1-dev \
                libmodplug-dev \
                libvpx-dev \
                libass-dev \
                libbluray-dev \
                libfribidi-dev \
		libgmp-dev \
                libgnutls28-dev \
                libmp3lame-dev \
                libopencore-amrnb-dev \
                libopencore-amrwb-dev \
                librtmp-dev \
                libtheora-dev \
                libx264-dev \
                libx265-dev \
                libxvidcore-dev \
                libfdk-aac-dev \
		libwebp-dev \
		build-essential \
		cmake \
		libtbb-dev \
		libxrandr-dev \
		libxcursor-dev \
		libxinerama-dev \
		libxi-dev \
		libglfw3-dev \
		libsfml-dev \
		libxcomposite-dev \
		libpangocairo-1.0-0 \
		libxss-dev \
		libatk1.0-dev \
		libcups2-dev \
		libgtk2.0-dev \
		libgdk-pixbuf2.0-dev


mkdir build_deps
cd build_deps

echo "Build Boost"

wget https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz
tar -zvxf boost_1_66_0.tar.gz
cd boost_1_66_0
./bootstrap.sh --prefix=/opt/boost
./b2 --with=all -j8 install


echo "Build FFMPEG"

cd ../
wget https://github.com/FFmpeg/FFmpeg/archive/acdea9e7c56b74b05c56b4733acc855b959ba073.tar.gz
tar -zxvf  acdea9e7c56b74b05c56b4733acc855b959ba073.tar.gz
cd FFmpeg-acdea9e7c56b74b05c56b4733acc855b959ba073
./configure \
                        --enable-version3 \
                        --enable-gpl \
                        --enable-nonfree \
                        --enable-small \
                        --enable-libmp3lame \
                        --enable-libx264 \
                        --enable-libx265 \
                        --enable-libvpx \
                        --enable-libtheora \
                        --enable-libvorbis \
                        --enable-libopus \
                        --enable-libfdk-aac \
                        --enable-libass \
                        --enable-libwebp \
                        --enable-librtmp \
                        --enable-postproc \
                        --enable-avresample \
                        --enable-libfreetype \
                        --enable-openssl \
                        --disable-debug \
                        --prefix=/opt/ffmpeg

make -j8
make install

echo "Copy CEF"

cd ../
wget http://opensource.spotify.com/cefbuilds/cef_binary_3.3239.1723.g071d1c1_linux64_minimal.tar.bz2 -O cef.tar.bz2

tar -jxvf cef.tar.bz2
mkdir /opt/cef
mv cef_binary_*/* /opt/cef

cd ..
mkdir build
cd build

echo "Build CasparCG"
export BOOST_ROOT=/opt/boost
export PKG_CONFIG_PATH=/opt/ffmpeg/lib/pkgconfig
cmake ../src
make -j8

cd ..
# Find a better way to copy deps
ln -s build/staging staging
src/shell/copy_deps.sh build/shell/casparcg staging/lib
echo "DONE - See staging/ for app"
