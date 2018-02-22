FROM casparcg/base:1.0.0 as BUILD
	ENV FFMPEG_COMMIT acdea9e7c56b74b05c56b4733acc855b959ba073

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
	RUN wget https://github.com/FFmpeg/FFmpeg/archive/${FFMPEG_COMMIT}.tar.gz && \
		tar zxf ${FFMPEG_COMMIT}.tar.gz
	WORKDIR FFmpeg-${FFMPEG_COMMIT}
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

FROM scratch
	COPY --from=BUILD /opt/ffmpeg /opt/ffmpeg
