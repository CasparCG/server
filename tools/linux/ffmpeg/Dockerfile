ARG IMAGE_BASE

FROM ${IMAGE_BASE} as BUILD
	ARG PROC_COUNT=8
	ENV FFMPEG_COMMIT n4.2.2
	ENV FFMPEG_NVCODEC_COMMIT n9.1.23.1

	RUN wget https://github.com/FFmpeg/nv-codec-headers/archive/${FFMPEG_NVCODEC_COMMIT}.tar.gz && \
		tar zxf ${FFMPEG_NVCODEC_COMMIT}.tar.gz
	RUN wget https://github.com/FFmpeg/FFmpeg/archive/${FFMPEG_COMMIT}.tar.gz && \
		tar zxf ${FFMPEG_COMMIT}.tar.gz

	WORKDIR /nv-codec-headers-${FFMPEG_NVCODEC_COMMIT}
	RUN make && make install

	WORKDIR /FFmpeg-${FFMPEG_COMMIT}
	# This is setup to match zeranoe ffmpeg builds for windows as closely as possible
	RUN ./configure \
			--enable-gpl \
			--enable-version3 \
			--enable-sdl2 \
			--enable-fontconfig \
			--enable-gnutls \
			--enable-iconv \
			--enable-libass \
			--enable-libbluray \
			--enable-libfreetype \
			--enable-libmp3lame \
			--enable-libopencore-amrnb \
			--enable-libopencore-amrwb \
			--enable-libopenjpeg \
			--enable-libopus \
			--enable-libshine \
			--enable-libsnappy \
			--enable-libsoxr \
			--enable-libtheora \
			--enable-libtwolame \
			--enable-libvpx \
			--enable-libwavpack \
			--enable-libwebp \
			--enable-libx264 \
			--enable-libx265 \
			--enable-libxml2 \
			--enable-lzma \
			--enable-zlib \
			--enable-gmp \
			--enable-libvorbis \
			--enable-libvo-amrwbenc \
			--enable-libspeex \
			--enable-libxvid \
			--enable-ffnvcodec \
			--enable-cuvid \
			--enable-nvenc \
			--enable-nvdec \
			--enable-avisynth \
			--enable-libopenmpt \
			--prefix=/opt/ffmpeg \
			&& \
		make -j $PROC_COUNT && \
		make install

	# These are not available in the ubuntu18.04 apt repos
			# --enable-libdav1d \
			# --enable-libzimg \
			# --enable-libvidstab \
			# --enable-libaom \
			# --enable-libmfx \
			# --enable-libmysofa \ # errors while building
			# --enable-amf \
FROM scratch
	COPY --from=BUILD /opt/ffmpeg /opt/ffmpeg
