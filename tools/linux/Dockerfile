ARG IMAGE_BASE
ARG IMAGE_FFMPEG

FROM ${IMAGE_FFMPEG} as ffmpeg

FROM ${IMAGE_BASE} as build-casparcg
	COPY --from=ffmpeg /opt/ffmpeg /opt/ffmpeg

	RUN mkdir /source && mkdir /build && mkdir /install

	RUN apt-get update && \
		apt-get install -y libboost-all-dev ; \
		rm -rf /var/lib/apt/lists/*

	COPY ./src /source

	WORKDIR /build

	ENV PKG_CONFIG_PATH=/opt/ffmpeg/lib/pkgconfig

	ARG CC
	ARG CXX
	ARG GIT_HASH

	RUN cmake -GNinja /source
	RUN ninja

	# Find a better way to copy deps
	RUN ln -s /build/staging /staging && \
		/source/shell/copy_deps.sh /staging/bin/casparcg /staging/lib

FROM nvidia/opengl:1.2-glvnd-devel-ubuntu22.04
	COPY --from=build-casparcg /staging /opt/casparcg

	RUN set -ex; \
			apt-get update; \
			DEBIAN_FRONTEND="noninteractive" apt-get install -y --no-install-recommends \
            tzdata \
			libc++1 \
			libnss3 \
			fontconfig \
			; \
			rm -rf /var/lib/apt/lists/*

	WORKDIR /opt/casparcg

	ADD tools/linux/run_docker.sh ./
	CMD ["./run_docker.sh"]
