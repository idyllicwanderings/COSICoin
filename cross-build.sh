# Set up the environment with the PYNQ toolchain
source ~micasusr/design/scripts/xilinx_vitis_2023.1.rc
# Make sure protoc and the latest cmake in path
export PATH="/freeware/gprc-1.61.1/x86/bin:/usr/bin:$PATH"
mkdir -p cmake/build && cd cmake/build
cmake -DCMAKE_TOOLCHAIN_FILE=/freeware/gprc-1.61.1/arm/arm.xilinx.toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release -Dabsl_DIR=/freeware/gprc-1.61.1/arm/lib/cmake/absl \
    -DProtobuf_DIR=/freeware/gprc-1.61.1/arm/lib/cmake/protobuf \
    -Dutf8_range_DIR=/freeware/gprc-1.61.1/arm/lib/cmake/utf8_range \
    -DgRPC_DIR=/freeware/gprc-1.61.1/arm/lib/cmake/grpc -DBUILD_TESTS=OFF ../..
make -j 8
