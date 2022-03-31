ARG IMAGE_BASE

FROM ${IMAGE_BASE} as BUILD
  ARG PROC_COUNT=8

  WORKDIR /opt/build

  RUN curl -sSL https://boostorg.jfrog.io/artifactory/main/release/1.78.0/source/boost_1_78_0.tar.gz \
      | tar zx --strip-components 1 && \
    ./bootstrap.sh --prefix=/opt/boost && \
    ./b2 --with=all -j $PROC_COUNT install || exit 0

FROM scratch
  COPY --from=BUILD /opt/boost /opt/boost
