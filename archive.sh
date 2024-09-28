#!/usr/bin/bash

tar -cf fduos.tar \
    src/          \
    .gdbinit      \
    CMakeLists.txt \
    Dockerfile    \
    fduos-addr2line \
    fduos-format \
    fduos-grep \
