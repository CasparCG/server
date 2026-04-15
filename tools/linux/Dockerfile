FROM docker.io/buildpack-deps:noble AS build-casparcg
	ADD tools/linux/install-dependencies /

	RUN apt-get update && /install-dependencies

	RUN mkdir /source && mkdir /build && mkdir /install

	COPY ./src /source

	WORKDIR /build

	ARG CC
	ARG CXX
	ARG GIT_HASH

	RUN cmake -GNinja /source -DUSE_STATIC_BOOST=ON -DUSE_SYSTEM_CEF=OFF

	RUN cmake --build .

	RUN cmake --install . --prefix staging

	# Find a better way to copy deps
	RUN ln -s /build/staging /staging && \
		/source/shell/copy_deps.sh /build/shell/casparcg /staging/lib

FROM docker.io/ubuntu:noble
	COPY --from=build-casparcg /staging /opt/casparcg

	RUN set -ex; \
			apt-get update; \
			DEBIAN_FRONTEND="noninteractive" apt-get install -y --no-install-recommends \
			tzdata \
			libc++1 \
			libnss3 \
			fontconfig \
			libglvnd0 \
			libgl1 \
			libglx0 \
			libegl1 \
			libgles2 \
			; \
			rm -rf /var/lib/apt/lists/*

	WORKDIR /opt/casparcg
	RUN chmod 755 run.sh
	ADD tools/linux/run_docker.sh ./
	CMD ["./run_docker.sh"]
