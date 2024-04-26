cmake_minimum_required(VERSION 3.8)

project(Chat C CXX)

include(cmake/common.cmake)

# Copy servers.txt
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/servers.txt" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

# Proto file
get_filename_component(hw_proto "protos/chat.proto" ABSOLUTE)
get_filename_component(hw_proto_path "${hw_proto}" PATH)

# Set STD standard to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)

# Generated sources
set(hw_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/chat.pb.cc")
set(hw_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/chat.pb.h")
set(hw_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/chat.grpc.pb.cc")
set(hw_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/chat.grpc.pb.h")
set(CMAKE_BUILD_TYPE "Debug")
add_custom_command(
      OUTPUT "${hw_proto_srcs}" "${hw_proto_hdrs}" "${hw_grpc_srcs}" "${hw_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hw_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_proto}"
      DEPENDS "${hw_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
include_directories("include")

# hw_grpc_proto
add_library(hw_grpc_proto
  ${hw_grpc_srcs}
  ${hw_grpc_hdrs}
  ${hw_proto_srcs}
  ${hw_proto_hdrs})
target_link_libraries(hw_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

# Make static libraries
# Libraries without GRPC
foreach(_target
  signature)
  file(GLOB TARGET_SOURCES "lib/${_target}/*.cc")
  add_library(${_target} STATIC ${TARGET_SOURCES})
  target_include_directories(${_target}
    PRIVATE
      ${PROJECT_SOURCE_DIR}/include
    )
endforeach()
# Libraries with GRPC
foreach(_target
  blockchain comms)
  file(GLOB TARGET_SOURCES "lib/${_target}/*.cc")
  add_library(${_target} STATIC ${TARGET_SOURCES})
  target_link_libraries(${_target}
    hw_grpc_proto
    absl::flags
    absl::flags_parse
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})
  target_include_directories(${_target}
    PRIVATE
      ${PROJECT_SOURCE_DIR}/include
    )
endforeach()

# Make executables
# foreach(_target
#   client_ex server_ex)
#   add_executable(${_target} "src/${_target}.cc")
#   target_link_libraries(${_target}
#     blockchain
#     comms
#     hashing
#     )
#   target_include_directories(${_target}
#     PRIVATE
#       ${PROJECT_SOURCE_DIR}/include
#     )
# endforeach()

# Make & run tests
enable_testing()
include(GoogleTest)

# Get a list of all .cc files in the test folder
file(GLOB TEST_SOURCES "test/*.cc")

# Iterate over the list and create an executable for each file
foreach(TEST_SOURCE ${TEST_SOURCES})
  # Get the file name without extension
  get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)

  # Create the executable
  add_executable(${TEST_NAME} ${TEST_SOURCE})

  # Link the necessary libraries
  target_link_libraries(${TEST_NAME}
    blockchain
    signature
    GTest::gtest_main
  )

  # Add the test to Google Test
  gtest_discover_tests(${TEST_NAME})
endforeach()
