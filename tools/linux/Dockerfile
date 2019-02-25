ARG IMAGE_BASE
ARG IMAGE_BOOST
ARG IMAGE_FFMPEG
ARG IMAGE_CEF

FROM ${IMAGE_BOOST} as boost
FROM ${IMAGE_FFMPEG} as ffmpeg
FROM ${IMAGE_CEF} as cef


FROM ${IMAGE_BASE} as build-casparcg
	COPY --from=boost /opt/boost /opt/boost
	COPY --from=ffmpeg /opt/ffmpeg /opt/ffmpeg
	COPY --from=cef /opt/cef /opt/cef

    ARG PROC_COUNT=8

	RUN mkdir /source && mkdir /build && mkdir /install

	COPY ./src /source

	WORKDIR /build

	ENV BOOST_ROOT=/opt/boost
	ENV PKG_CONFIG_PATH=/opt/ffmpeg/lib/pkgconfig

    ARG CC
    ARG CXX
	ARG GIT_HASH

	RUN cmake /source
	RUN make -j $PROC_COUNT

	# Find a better way to copy deps
	RUN ln -s /build/staging /staging && \
		/source/shell/copy_deps.sh shell/casparcg /staging/lib

FROM ubuntu:bionic
	COPY --from=build-casparcg /staging /opt/casparcg

	# This can be removed once bionic is more stable, as this image currently has an older version of libc than buildpack-deps:bionic
	RUN apt-get update && apt-get dist-upgrade -y && rm -rf /var/lib/apt/lists/*

	WORKDIR /opt/casparcg
	ENTRYPOINT ["/opt/casparcg/run.sh"]
