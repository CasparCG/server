FROM casparcg/base:1.0.0 as BUILD
	ENV LIBBOOST_VERSION 1_66_0

	RUN wget https://dl.bintray.com/boostorg/release/1.66.0/source/boost_${LIBBOOST_VERSION}.tar.gz && \
		tar zxf boost_${LIBBOOST_VERSION}.tar.gz && \
		cd boost_${LIBBOOST_VERSION} && \
		./bootstrap.sh --prefix=/opt/boost && \
		./b2 --with=all -j8 install || exit 0

FROM scratch
	COPY --from=BUILD /opt/boost /opt/boost
