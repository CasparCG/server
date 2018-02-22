FROM casparcg/base:1.0.0 as BUILD
	ADD http://opensource.spotify.com/cefbuilds/cef_binary_3.3239.1723.g071d1c1_linux64_minimal.tar.bz2 /opt/cef.tar.bz2
	WORKDIR /opt
	RUN tar -jxf cef.tar.bz2 && mv /opt/cef_binary_* /opt/cef

FROM scratch
	COPY --from=BUILD /opt/cef /opt/cef
