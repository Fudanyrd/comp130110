# syntax=docker/dockerfile:1
FROM ubuntu:22.04

WORKDIR /home/yrd
RUN apt-get -y update && apt-get -y upgrade
RUN apt-get -y install gcc-aarch64-linux-gnu
RUN apt-get -y install gdb-multiarch
RUN apt-get -y install qemu-system-arm
RUN apt-get -y install ninja-build
RUN apt-get -y install cmake
RUN apt-get -y install dosfstools
RUN apt-get -y install mtools
RUN apt-get -y install fdisk

CMD ["sleep", "infinity"]
