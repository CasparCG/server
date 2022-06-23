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

FROM nvidia/opengl:1.2-glvnd-devel-ubuntu20.04
	COPY --from=build-casparcg /staging /opt/casparcg

	RUN set -ex; \
		apt-get update; \    
		DEBIAN_FRONTEND="noninteractive" apt-get install -y --no-install-recommends tzdata; \
		apt-get install -y --no-install-recommends \
			libc++1 \
			libnss3 \
			fontconfig \
			libxrandr2 \
			libogg-dev \
			libcairo-dev \
			libnuma1 \
			libgomp1 \      
			libvorbisfile3 \
			libxinerama1 \
			libxi6 \
			libxss1 \
			libwayland-egl1-mesa \
			libwayland-cursor0 \
			libxkbcommon-dev \
			libfribidi-dev \
			libharfbuzz-dev \
			libicu66 \
			libjxr-dev \
			libjpeg-dev \
			libraw-dev \
			libtiff-dev \
			libopenexr24 \
			libxcomposite-dev \
			libxdamage1 \
			libxtst-dev \
			libpango-1.0-0 \
			libpangocairo-1.0-0 \
			libatk1.0-0 \
			libatk-bridge2.0-0 \
			libxcursor1 \ 
			mpg123 \
			; \
			rm -rf /var/lib/apt/lists/*

	WORKDIR /opt/casparcg

	ADD tools/linux/run_docker.sh ./
	CMD ["./run_docker.sh"]
