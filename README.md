# COSICOIN

## Building the code

Execute the following four commands to build the code.

```bash
mkdir -p cmake/build
pushd cmake/build
cmake -DCMAKE_PREFIX_PATH=$HOME/.local ../..
make -j 4
```

Note: this code assumes the GRPC install directory is in `$HOME/.local`!

Note: the `build.sh` script runs those four commands at once.

## Running the code

`cd` to `cmake/build` and run the executables defined in the `src` directory.

## References

We used the sha256 implementation from this repository: [https://github.com/stbrumme/hash-library](https://github.com/stbrumme/hash-library).
