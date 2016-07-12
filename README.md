# koruza-driver

## Build instructions

In order to build and run the unit tests:
```
mkdir build
cd build
cmake -DONLY_TESTS=TRUE ..
make
make test
```

Building the full driver requires the OpenWrt toolchain.
