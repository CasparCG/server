ARG IMAGE_BASE

FROM ${IMAGE_BASE} as BUILD
ARG PROC_COUNT=8
ENV LIBBOOST_VERSION 1_67_0

RUN wget https://boostorg.jfrog.io/artifactory/main/release/1.67.0/source/boost_1_67_0.tar.gz
RUN tar zxf boost_${LIBBOOST_VERSION}.tar.gz
WORKDIR /boost_${LIBBOOST_VERSION}
RUN ./bootstrap.sh --prefix=/opt/boost
RUN ./b2 --with=all -j $PROC_COUNT install

# Patch a compatibility issue with GCC11
WORKDIR /opt/boost
ADD https://github.com/boostorg/concept_check/commit/affc79e04514b4cc4177967ced85f7674f3a6642.patch /boost-concept-gcc11.patch
RUN patch -p1 < /boost-concept-gcc11.patch

FROM scratch
COPY --from=BUILD /opt/boost /opt/boost
