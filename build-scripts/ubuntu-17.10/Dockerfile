FROM buildpack-deps:artful

RUN set -ex; \
	apt-get update; \
	apt-get install -y --no-install-recommends \
		clang \
		cmake \
		libxrandr-dev \
		libjpeg-dev \
		libsndfile1-dev \
		libudev-dev \
		libglu1-mesa-dev \
		libharfbuzz0b \
		libpangoft2-1.0-0 \
		libcairo2 \
		libv4l-0 \
		libraw1394-11 \
		libavc1394-0 \
		libiec61883-0 \
		libxtst6 \
		libnss3 \
		libnspr4 \
		libgconf-2-4 \
		libasound2 \
		libboost-all-dev \
		ffmpeg \
		libavdevice-dev \
		libfreeimage-dev \
		libglew-dev \
		libtbb-dev \
		libopenal-dev \
		libglfw3-dev \
		libsfml-dev \
		libopus-dev \
		libgsm1-dev \
		libmodplug-dev \
		libvpx-dev \
		libass-dev \
		libbluray-dev \
		libfribidi-dev \
		libgmp-dev \
		libgnutls28-dev \
		libmp3lame-dev \
		libopencore-amrnb-dev \
		libopencore-amrwb-dev \
		librtmp-dev \
		libtheora-dev \
		libx264-dev \
		libxvidcore-dev \
		libxcursor-dev \
		lsb-release \
		libxinerama-dev \
		libxi-dev \
		libnih-dbus-dev \
	; \
	rm -rf /var/lib/apt/lists/*

#RUN update-alternatives --set c++ /usr/bin/clang++; \
#    update-alternatives --set cc /usr/bin/clang

RUN mkdir -p /build/products; mkdir /source

WORKDIR /build
