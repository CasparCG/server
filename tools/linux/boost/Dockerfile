ARG IMAGE_BASE

FROM ${IMAGE_BASE} as BUILD
    ARG PROC_COUNT=8
	ENV LIBBOOST_VERSION 1_67_0

	RUN wget https://boostorg.jfrog.io/artifactory/main/release/1.67.0/source/boost_1_67_0.tar.gz && \
		tar zxf boost_${LIBBOOST_VERSION}.tar.gz && \
		cd boost_${LIBBOOST_VERSION} && \
		./bootstrap.sh --prefix=/opt/boost && \
		./b2 --with=all -j $PROC_COUNT install || exit 0

FROM scratch
	COPY --from=BUILD /opt/boost /opt/boost
