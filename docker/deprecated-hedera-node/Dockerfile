FROM ubuntu:bionic
WORKDIR /opt
#COPY . .
RUN apt update && apt install -y git cmake
RUN git clone https://github.com/gradido/gn.git
RUN cd gn && ./gen-proto.sh && mkdir build && cd build && cmake .. && make -j8
#CMD ["node", "src/index.js"]
CMD ["ls", "-al"]