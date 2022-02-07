From ubuntu:18.04


RUN apt-get update && apt-get install -y build-essential libbz2-dev libgtest-dev gpg wget software-properties-common

RUN wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | tee /usr/share/keyrings/kitware-archive-keyring.gpg >/dev/null
RUN echo 'deb [signed-by=/usr/share/keyrings/kitware-archive-keyring.gpg] https://apt.kitware.com/ubuntu/ bionic main' | tee /etc/apt/sources.list.d/kitware.list >/dev/null
RUN add-apt-repository ppa:ubuntu-toolchain-r/test && apt-get update && apt-get install -y cmake libbz2-dev libgtest-dev gcc-9 g++-9
RUN cd /usr/src/googletest/googletest && mkdir build && cd build && cmake .. && make && cp libgtest* /usr/lib && cd .. && rm -rf build

ENV CC=/usr/bin/gcc-9
ENV CXX=/usr/bin/g++-9

ENTRYPOINT ["tail","-f","/dev/null"]
