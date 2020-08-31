cmake_minimum_required(VERSION 3.0)
project(Gradido_Node C CXX)

add_definitions("-std=c++11" "-pthread")
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

include_directories(
    "lib/include"
    "proto_gen"
    "src"
)
link_directories(
    "lib"
)

FILE(GLOB LOCAL_SRC "src/*.cpp" "src/*.h")
FILE(GLOB PROTOS "proto_gen/*.cc" "proto_gen/*.h")
FILE(GLOB PROTOS_MIRROR "proto_gen/mirror/*.cc" "proto_gen/mirror/*.h")
FILE(GLOB PROTOS_GRADIDO "proto_gen/gradido/*.cc" "proto_gen/gradido/*.h")

add_executable(gradido_node ${LOCAL_SRC} ${PROTOS} ${PROTOS_MIRROR} ${PROTOS_GRADIDO})
target_link_libraries(gradido_node -led25519 -lPocoNet -lPocoUtil -lPocoJSON -lPocoFoundation -lPocoData -lPocoNetSSL -lall -ldl -lpthread)

add_executable(dump_blockchain "src/blockchain_gradido_def.h" "src/utils.h" "src/utils.cpp" "src/dump_blockchain/dump_blockchain.cpp")
target_link_libraries(dump_blockchain -lPocoNet -lPocoUtil -lPocoJSON -lPocoFoundation -lPocoData -lPocoNetSSL -lpthread)


