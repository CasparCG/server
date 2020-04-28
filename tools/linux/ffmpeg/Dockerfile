ARG IMAGE_BASE

FROM ${IMAGE_BASE} as BUILD
    ARG PROC_COUNT=8
	ENV FFMPEG_COMMIT n4.2.2

	RUN wget https://github.com/FFmpeg/FFmpeg/archive/${FFMPEG_COMMIT}.tar.gz && \
		tar zxf ${FFMPEG_COMMIT}.tar.gz
	WORKDIR /FFmpeg-${FFMPEG_COMMIT}
	RUN ./configure \
			--enable-version3 \
			--enable-gpl \
			--enable-small \
			--enable-libmp3lame \
			--enable-libx264 \
			--enable-libx265 \
			--enable-libvpx \
			--enable-libtheora \
			--enable-libvorbis \
			--enable-libopus \
			--enable-libass \
			--enable-libwebp \
			--enable-librtmp \
			--enable-postproc \
			--enable-avresample \
			--enable-libfreetype \
			--enable-gnutls \
			--disable-debug \
			--prefix=/opt/ffmpeg \
			&& \
		make -j $PROC_COUNT && \
		make install

FROM scratch
	COPY --from=BUILD /opt/ffmpeg /opt/ffmpeg
