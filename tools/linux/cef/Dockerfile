ARG IMAGE_BASE

FROM ${IMAGE_BASE} as BUILD
    ARG PROC_COUNT=8

	ADD https://cef-builds.spotifycdn.com/cef_binary_3.3578.1870.gc974488_linux64_minimal.tar.bz2 /opt/cef.tar.bz2
	WORKDIR /opt
	RUN tar -jxf cef.tar.bz2 && mv /opt/cef_binary_* /opt/cef
    RUN mkdir /opt/build
    WORKDIR /opt/build
    RUN cmake /opt/cef -DCEF_CXX_COMPILER_FLAGS="-Wno-attributes" && make -j $PROC_COUNT
    RUN cp /opt/build/libcef_dll_wrapper/libcef_dll_wrapper.a /opt/cef/Release/
    RUN strip /opt/cef/Release/libcef.so

FROM scratch
	COPY --from=BUILD /opt/cef /opt/cef
