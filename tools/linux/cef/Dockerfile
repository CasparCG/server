ARG IMAGE_BASE

FROM ${IMAGE_BASE} as BUILD
  ARG PROC_COUNT=8

  # install newer cmake
  RUN apt update && apt install -y software-properties-common lsb-release && \
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null && \
    apt-add-repository "deb https://apt.kitware.com/ubuntu/ $(lsb_release -cs) main" && \
    apt update && apt install -y cmake

  WORKDIR /opt/cef
  RUN \
    curl -sSL https://cef-builds.spotifycdn.com/cef_binary_99.2.10%2Bgfecf7ca%2Bchromium-99.0.4844.51_linux64_minimal.tar.bz2 \
      | tar -jx --strip-components 1 && \
    cmake -S . -B /build && \
    cmake --build /build && \
    cp /build/libcef_dll_wrapper/libcef_dll_wrapper.a /opt/cef/Release/ && \
    strip /opt/cef/Release/libcef.so

FROM scratch
    COPY --from=BUILD /opt/cef /opt/cef
