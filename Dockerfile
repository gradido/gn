FROM ubuntu:bionic
WORKDIR /opt
RUN apt update && apt install -y apt-utils cmake build-essential ccache
COPY . gn
RUN [ -d "./gn/build" ] || (cd gn && ls && ./gen-proto.sh && mkdir build && cd build && cmake .. && make -j`nproc`)

# has to be provided as a volume; it should contain gradido.conf and
# siblings.txt
WORKDIR /opt/instance
EXPOSE 13000-14000

