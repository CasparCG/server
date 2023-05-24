FROM buildpack-deps:focal

ADD install-dependencies /

RUN chmod +x /install-dependencies
RUN apt-get update && \
	/install-dependencies ; \
	rm -rf /var/lib/apt/lists/*

