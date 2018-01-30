#!/bin/bash
#compile ffmpeg successful on ubuntu 14.04.3 & 16.04.1 amd64 using the CasparCG build requirments described here:
#https://raw.githubusercontent.com/casparcg/Server/2.1.0/BUILDING

sudo apt-get -y install curl tar pkg-config python2.7-dev zlib1g-dev autoconf libtool subversion libfontconfig1-dev ant default-jre default-jdk frei0r-plugins-dev libv4l-dev libiec61883-dev libavc1394-dev

./build.sh
