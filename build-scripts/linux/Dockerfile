FROM casparcg/base:1.0.0
	COPY --from=casparcg/boost:1.66 /opt/boost /opt/boost
	COPY --from=casparcg/ffmpeg:3.4.1 /opt/ffmpeg /opt/ffmpeg

	RUN mkdir /source && mkdir /build && mkdir /install

	COPY ./src /source

	WORKDIR /build

	ENV BOOST_ROOT=/opt/boost
	ENV PKG_CONFIG_PATH=/opt/ffmpeg/lib/pkgconfig
	RUN cmake /source
	RUN make -j8

	# Find a better way to copy deps

	RUN mkdir -p /staging/lib && mkdir -p /staging/bin && \
		/source/shell/copy_deps.sh shell/casparcg /staging/lib && \
		cp shell/casparcg /staging/bin/casparcg && \
		cp /source/shell/casparcg.config /staging/casparcg.config && \
		cp /source/shell/run.sh /staging/run.sh && \
		cp /source/shell/liberation-fonts/LiberationMono-Regular.ttf /staging/LiberationMono-Regular.ttf

FROM ubuntu:artful
	COPY --from=build-casparcg /staging /opt/casparcg
	WORKDIR /opt/casparcg

