FROM casparcg/base:1.1.0 as build-casparcg
	COPY --from=casparcg/boost:1.66-1 /opt/boost /opt/boost
	COPY --from=casparcg/ffmpeg:master-acdea9e-1 /opt/ffmpeg /opt/ffmpeg
	COPY --from=casparcg/cef:3.3239.1723-1 /opt/cef /opt/cef

	RUN mkdir /source && mkdir /build && mkdir /install

	COPY ./src /source

	WORKDIR /build

	ENV BOOST_ROOT=/opt/boost
	ENV PKG_CONFIG_PATH=/opt/ffmpeg/lib/pkgconfig
	RUN cmake /source
	RUN make -j8

	# Find a better way to copy deps
	RUN ln -s /build/staging /staging && \
		/source/shell/copy_deps.sh shell/casparcg /staging/lib

FROM ubuntu:artful
	COPY --from=build-casparcg /staging /opt/casparcg
	WORKDIR /opt/casparcg
