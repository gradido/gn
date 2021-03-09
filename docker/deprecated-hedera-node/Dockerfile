# TODO: require 64 bit arch
# TODO: inherit
# TODO: unittests
# TODO: dockerfiles by name (?)
FROM ubuntu:bionic
WORKDIR /opt
RUN apt update && apt install -y apt-utils git cmake build-essential ccache
RUN git clone https://github.com/gradido/gn.git
RUN cd gn && git checkout --track origin/master && ./gen-proto.sh && mkdir build && cd build && cmake .. && make -j`nproc`

# has to be provided as a volume; it should contain gradido.conf and
# siblings.txt
WORKDIR /opt/instance
EXPOSE 13000-14000
CMD exec ../gn/build/gradido_deprecated 2>>err.log

