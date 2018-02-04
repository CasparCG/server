FROM buildpack-deps:artful as build-libboost
ENV LIBBOOST_VERSION 1_66_0
COPY build-scripts/linux/files/apt-lists/ /var/lib/apt/lists
RUN wget https://dl.bintray.com/boostorg/release/1.66.0/source/boost_${LIBBOOST_VERSION}.tar.gz && \
	tar zxf boost_${LIBBOOST_VERSION}.tar.gz && \
	cd boost_${LIBBOOST_VERSION} && \
	./bootstrap.sh --prefix=/opt/boost && \
	./b2 --with=all -j8 install || exit 0

FROM buildpack-deps:artful as build-ffmpeg
ENV FFMPEG_VERSION 3.4.1
COPY build-scripts/linux/files/apt-lists/ /var/lib/apt/lists
# Cleanup this list below!!
RUN apt-get install -yq --no-install-recommends \
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
	libfdk-aac-dev
RUN wget http://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.gz && \
	tar zxf ffmpeg-${FFMPEG_VERSION}.tar.gz && \
	cd ffmpeg-${FFMPEG_VERSION}
WORKDIR ffmpeg-${FFMPEG_VERSION}
RUN ./configure \
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
		--prefix=/opt/ffmpeg \
		&& \
	make -j8 && \
	make install

FROM buildpack-deps:artful as build-casparcg
COPY build-scripts/linux/files/apt-lists/ /var/lib/apt/lists
# Cleanup this list below!!
RUN apt-get install -yq --no-install-recommends \
	cmake \
	lsb-release \
	libglew-dev \
	libfreeimage-dev \
	libtbb-dev \
	libsndfile1-dev \
	libopenal-dev \
	libglfw3-dev \
	libxcursor-dev \
	libxinerama-dev \
	libxi-dev \
	libnih-dbus-dev \
	libsfml-dev \
	libxrandr-dev \
	libfdk-aac-dev \
	libjpeg-dev libsndfile1-dev libudev-dev libglu1-mesa-dev libharfbuzz0b libpangoft2-1.0-0 libcairo2 libv4l-0 libraw1394-11 libavc1394-0 libiec61883-0 libxtst6 libnss3 libnspr4 libgconf-2-4 libasound2 libfreeimage-dev libglew-dev libtbb-dev libopenal-dev libglfw3-dev libsfml-dev libopus-dev libgsm1-dev libmodplug-dev libvpx-dev libass-dev libbluray-dev libfribidi-dev libgmp-dev libgnutls28-dev libmp3lame-dev libopencore-amrnb-dev libopencore-amrwb-dev librtmp-dev libtheora-dev libx264-dev libxvidcore-dev libx265-dev
COPY --from=build-libboost /opt/boost /opt/boost
COPY --from=build-ffmpeg /opt/ffmpeg /opt/ffmpeg
RUN mkdir /source && mkdir /build && mkdir /install
COPY accelerator /source/accelerator
COPY common /source/common
COPY core /source/core
COPY modules /source/modules
COPY protocol /source/protocol
COPY resources /source/resources
COPY shell /source/shell
COPY CMakeModules /source/CMakeModules
COPY CMakeLists.txt /source/CMakeLists.txt
COPY version.h /source/version.h
COPY version.tmpl /source/version.tmpl
COPY CHANGELOG /source/CHANGELOG
COPY LICENSE /source/LICENSE
COPY build-scripts/linux/files/RxCpp/include/ /usr/local/include/
WORKDIR /build
ENV BOOST_ROOT=/opt/boost
ENV PKG_CONFIG_PATH=/opt/ffmpeg/lib/pkgconfig
RUN cmake /source
RUN make -j8
# Find a better way to copy deps
COPY build-scripts/linux/files/copy_deps.sh /build/copy_deps.sh
RUN mkdir -p /staging/lib && mkdir -p /staging/bin && \
	./copy_deps.sh shell/casparcg /staging/lib && \
	cp shell/casparcg /staging/bin/casparcg && \
	cp /source/shell/casparcg.config /staging/casparcg.config && \
	cp /source/shell/run.sh /staging/run.sh && \
	cp /source/shell/LiberationSans-Regular.ttf /staging/LiberationSans-Regular.ttf

FROM ubuntu:artful
COPY --from=build-casparcg /staging /opt/casparcg
WORKDIR /opt/casparcg
