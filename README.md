# COSICOIN

## Programming Library Prerequisites

Requires a C++*17* or higher version.

gRPC: https://grpc.io/

gTest: https://github.com/google/googletest

gLog: https://github.com/google/glog

the sha256 implementation: [https://github.com/stbrumme/hash-library](https://github.com/stbrumme/hash-library).

nlohmann: https://github.com/nlohmann/json

## Building the code

Execute the following four commands to build and test the code locally.

```bash
mkdir -p cmake/build
pushd cmake/build
cmake -DCMAKE_PREFIX_PATH=/freeware/grpc-1.34 ../..
make -j 4
ctest
```

Note: this code assumes the GRPC install directory is in `$HOME/.local`!

Note: the `build.sh` script runs those commands at once.



## Cross compiling the code on FPGA boards

Execute the `cross-build.sh` script to cross compile for the PYNQ boards.


## The executables

After building the following executables are made:

- `server_client_test` (tests simple localhost server client communication)
- `node_test_ex` (tests consensus protocol locally)
- `hash_test_ex` (tests calculating hash using hardware, has to be run with `sudo` on the board)
- `hash_timings` (compares the running time of calculating a hash in software or hardware, has to be run with `sudo` on the board)
- `demo` (runs a full demo locally)

The demo runs correctly until the verifying stage. That means the consensus is reached but because of some unsolved exception the block is never added to the blockchain and thus the utxolist is not updated.

## Executing on Software

`cd` to `cmake/build` and run the executables defined in the `src` directory.

`cd` to `cmake/build` and run the test scripts with `ctest`.

## Hardware

To run the hardware the bitstream must first be flashed on the fpga. This can be done by putting the files hw.bit.bin and fpgautils on the board, and running the following 3 commands:

```bash
sudo chown –R xilinx:xilinx ./*
sudo chmod +x fpgautils
sudo ./fpgautil -b hw.bit.bin -f Full
```

Now the FPGA is programmed correctly and ready to calculate some hashes

The hardware can also be tested seperately by running cmake . and make. The source code of the executionable can be found in main.c The block has to be initialised manually and the num_blocks should be zero if only one sha256 must be computed and one if two cycles are needed. To run the executionable you can use the following command:

```bash
sudo ./main
```

The files that were not written by us are fpgautils and the converter from a bitstream.bit to a bistream.bit.bin. All the other files in the folder verilog_code are written by us.

The modified verilog files are also provided. They can be placed directly in the interface of DDP.

## TODO

logging module(glog) unavailable for cross-compiling

database employment
