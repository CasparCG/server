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

	RUN wget http://download.qt.io/official_releases/qt/5.10/5.10.0/qt-opensource-linux-x64-5.10.0.run
	COPY ./tools/linux/qt-installer.js /qt-installer.js
	RUN chmod +x qt-opensource-linux-x64-5.10.0.run && ./qt-opensource-linux-x64-5.10.0.run --platform minimal --script /qt-installer.js

	RUN mkdir /source && mkdir /build && mkdir /install

	COPY ./src /source

	WORKDIR /build

	ENV BOOST_ROOT=/opt/boost
	ENV PKG_CONFIG_PATH=/opt/ffmpeg/lib/pkgconfig
	ENV Qt5Quick_DIR=/opt/Qt/5.10.0/gcc_64/lib/cmake/

    ARG CC
    ARG CXX
	ARG GIT_HASH

	RUN cmake /source
	RUN make -j $PROC_COUNT

	# Find a better way to copy deps
	RUN ln -s /build/staging /staging && \
		/source/shell/copy_deps.sh shell/casparcg /staging/lib

FROM nvidia/opengl:1.0-glvnd-devel-ubuntu18.04
	COPY --from=build-casparcg /staging /opt/casparcg

	RUN set -ex; \
		apt-get update; \
		apt-get install -y --no-install-recommends \
			libc++1 \
			libnss3 \
			fontconfig \
		; \
		rm -rf /var/lib/apt/lists/*

	WORKDIR /opt/casparcg

	ADD tools/linux/run_docker.sh ./
	CMD ["./run_docker.sh"]
