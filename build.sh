mkdir -p cmake/build
pushd cmake/build
cmake -DCMAKE_PREFIX_PATH=/freeware/grpc-1.34 -DBUILD_TESTS=ON ../..
make -j 4
ctest
