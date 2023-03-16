# LWAPIBindingGenerator

Simple utility for generating .asm bindings for usage in LWAPI.

Usage:

```
LWAPIBindingGenerator foo.h foo.asm
```

## Building

First, download and build [https://github.com/deech/libclang-static-build](libclang-static):

```
mkdir depends && cd depends
git clone https://github.com/deech/libclang-static-build.git
cd libclang-static-build
mkdir build && cd build
cmake .. -Thost=x64 -G "Visual Studio 16 2019" -A x64 -DCMAKE_INSTALL_PREFIX=.. -DCMAKE_BUILD_TYPE=Release -DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD="AVR" -DLLVM_ENABLE_LIBXML2=OFF -DLLVM_USE_CRT_RELEASE=MT
cmake --build . --config Release
cmake --install . --config Release
```

Then, build LWAPIBindingsGenerator:

```
cd ../../../
mkdir build && cd build
cmake .. -Thost=x64 -A x64
cmake --build . --config Release
```
