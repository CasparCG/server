ARG IMAGE_BASE

FROM ${IMAGE_BASE} as BUILD
    ARG PROC_COUNT=8

	WORKDIR /opt

	ADD https://cdn-fastly.obsproject.com/downloads/cef_binary_4638_linux64.tar.bz2 /opt/cef.tar.bz2
	RUN tar -jxf cef.tar.bz2 && mv /opt/cef_binary_* /opt/cef
    RUN mkdir /opt/build
    WORKDIR /opt/build
    RUN cmake /opt/cef && make -j $PROC_COUNT
    RUN cp /opt/build/libcef_dll_wrapper/libcef_dll_wrapper.a /opt/cef/Release/
    RUN strip /opt/cef/Release/libcef.so

FROM scratch
	COPY --from=BUILD /opt/cef /opt/cef
