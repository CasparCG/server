FROM casparcg/base:1.2.0 as build-casparcg
	COPY --from=casparcg/boost:1.66-2 /opt/boost /opt/boost
	COPY --from=casparcg/ffmpeg:master-acdea9e-2 /opt/ffmpeg /opt/ffmpeg
	COPY --from=casparcg/cef:3.3239.1723-2 /opt/cef /opt/cef

	RUN mkdir /source && mkdir /build && mkdir /install

	COPY ./src /source

	WORKDIR /build

	ENV BOOST_ROOT=/opt/boost
	ENV PKG_CONFIG_PATH=/opt/ffmpeg/lib/pkgconfig

	ARG GIT_HASH

	RUN cmake /source
	RUN make -j8

	# Find a better way to copy deps
	RUN ln -s /build/staging /staging && \
		/source/shell/copy_deps.sh shell/casparcg /staging/lib

FROM ubuntu:bionic
	COPY --from=build-casparcg /staging /opt/casparcg

	# This can be removed once bionic is more stable, as this image currently has an older version of libc than buildpack-deps:bionic
	RUN apt-get update && apt-get dist-upgrade -y && rm -rf /var/lib/apt/lists/*

	WORKDIR /opt/casparcg
	ENTRYPOINT ["/opt/casparcg/run.sh"]
