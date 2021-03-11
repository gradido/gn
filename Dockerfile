FROM ubuntu:bionic
WORKDIR /opt
RUN apt update && apt install -y apt-utils cmake build-essential ccache
COPY . gn
RUN [ -d "./gn/build" ] || (cd gn && ls && ./gen-proto.sh && mkdir build && cd build && cmake .. && ([ -z "$GN_MUTEX_LOG" ] && make -j`nproc` || make CXX_FLAGS=-DGN_MUTEX_LOG=1 -j`nproc`))

# has to be provided as a volume; it should contain gradido.conf and
# siblings.txt
WORKDIR /opt/instance
EXPOSE 13000-14000

