FROM docker.io/buildpack-deps:jammy as build-casparcg
	ADD tools/linux/install-dependencies /

	RUN apt-get update && /install-dependencies

	RUN mkdir /source && mkdir /build && mkdir /install

	COPY ./src /source

	WORKDIR /build

	ARG CC
	ARG CXX
	ARG GIT_HASH

	RUN cmake -GNinja /source -DUSE_STATIC_BOOST=ON -DUSE_SYSTEM_FFMPEG=OFF -DUSE_SYSTEM_CEF=OFF

	# ensure ffmpeg dependencies are installed
	RUN apt-get update && ./_deps/ffmpeg-lib-src/ffmpeg/install-ffmpeg-dependencies

	RUN ninja

	# Find a better way to copy deps
	RUN ln -s /build/staging /staging && \
		/source/shell/copy_deps.sh /staging/bin/casparcg /staging/lib

FROM docker.io/nvidia/opengl:1.2-glvnd-devel-ubuntu22.04
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
