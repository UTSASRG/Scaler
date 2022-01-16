# Build Guide

Check https://github.com/protocolbuffers/protobuf/blob/master/cmake/README.md about how to build.

On linux platform, the default makefile will work.

We must add -fPIC

So, compile command should look like this.

```
 cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fpic ${CMAKE_CXX_FLAGS}" -DCMAKE_C_FLAGS="-fpic ${CMAKE_C_FLAGS}" -DCMAKE_INSTALL_PREFIX=../../../../install ../..
```

Follow https://stackoverflow.com/questions/53651181/cmake-find-protobuf-package-in-custom-directory to link.


