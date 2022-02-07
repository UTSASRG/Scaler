# Build Guide

Check https://github.com/protocolbuffers/protobuf/blob/master/cmake/README.md about how to build.

On linux platform, the default makefile will work.

We must add -fPIC

So, compile command should look like this.

```
 cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-fpic ${CMAKE_CXX_FLAGS}" -DCMAKE_C_FLAGS="-fpic ${CMAKE_C_FLAGS}" -DCMAKE_INSTALL_PREFIX=../../../../install ../..
```

Follow https://stackoverflow.com/questions/53651181/cmake-find-protobuf-package-in-custom-directory to link.

Compiled files must be put into this folder in order to leverage existing build scripts (You don't have to build all build types. One is enough. But you might need to modify scripts in proto/CMakeLists.txt to let it use the correct one. By default, other scripts use binaries in the release build):
```
proto/lib/grpc_1_43_0/
├── debug
├── grpc_1_43_0.tar.xz
├── ReadMe.md
├── release
└── relwithdebinfo

```

# Pre-Built libraries

If the folder of grpc_{VERSION} is empty (except .gitignore and ReadMe.md). Current scripts will try to download pre-built binaries from https://driveus.xttech.top/s/BLCwmQdz32LA347/
If the link gets expired, you can contact jtang@umass.edu for renewal.