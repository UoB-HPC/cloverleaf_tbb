# A C++ TBB port of CloverLeaf

This is a port of [CloverLeaf](https://github.com/UoB-HPC/cloverleaf_kokkos) from MPI+Kokkos to
MPI+TBB

## Tested configurations

The program was compiled and tested on the following configurations.

## Building

Prerequisites:

* CentOS 7
* cmake3
* openmpi, opemmpi-devel
* [devtoolset-7](https://www.softwarecollections.org/en/scls/rhscl/devtoolset-7/)

**NOTE: TBB is downloaded and compiled as part of the CMake build, system TBB (if installed) will not be used.**

First, generate a build:

    cmake3 -Bbuild -H. -DCMAKE_BUILD_TYPE=Release  

Flags:

* `MPI_AS_LIBRARY` - `BOOL(ON|OFF)`, enable if CMake is unable to detect the correct MPI
  implementation or if you want to use a specific MPI installation. Use this a last resort only as
  your MPI implementation may pass on extra linker flags.
    * Set `MPI_C_LIB_DIR` to  <mpi_root_dir>/lib
    * Set `MPI_C_INCLUDE_DIR` to  <mpi_root_dir>/include
    * Set `MPI_C_LIB` to the library name, for exampe: mpich for libmpich.so
* `CXX_EXTRA_FLAGS` - `STRING`, appends extra flags that will be passed on to the compiler, applies
  to all configs
* `CXX_EXTRA_LINKER_FLAGS` - `STRING`, appends extra linker flags (the comma separated list after
  the `-Wl` flag) to the linker, applies to all configs
* `USE_VECTOR` - `BOOL(ON|OFF)`, whether to use `std::vector` for the backing buffers or plain pointers through `std::malloc`.  
 

If parts of your toolchain are installed at different places, you'll have to specify it manually,
for example:

    cmake3 -Bbuild -H.  \
    -DCMAKE_CXX_COMPILER=/PATH/TO/g++ \
    -DCMAKE_BUILD_TYPE=Release \

Proceed with compiling:

    cmake3 --build build --target clover_leaf --config Release -j $(nproc)

## Running

The main `clover_leaf` executable takes a `clover.in` file as parameter and outputs `clover.out` at
working directory.

For example, after successful compilation, at **project root**:

    ./build/clover_leaf --file InputDecks/clover_bm16_short.in

See [Tested configurations](#tested-configurations) for tested platforms and drivers.

For help, use the `-h` flag:

```
Options:
  -h  --help               Print the message
      --list               List available devices
      --device <INDEX>     Select device at INDEX from output of --list
      --file               Custom clover.in file (defaults to clover.in if unspecified)
```

