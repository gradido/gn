cmake_minimum_required(VERSION 3.0)
project(Gradido_Node C CXX)
add_definitions("-std=c++17" "-pthread" "-g")
set(CMAKE_CXX_STANDARD 17)
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
FILE(GLOB UNITTEST_SRC "unittest/*.cpp" "unittest/*.h")


add_executable(gradido_node ${LOCAL_SRC} ${PROTOS} ${PROTOS_MIRROR} ${PROTOS_GRADIDO} "src/gradido_node/main.cpp")
target_link_libraries(gradido_node -led25519 -lPocoNet -lPocoUtil -lPocoJSON -lPocoFoundation -lPocoData -lPocoNetSSL -lall -ldl -lpthread)

add_executable(dump_blockchain "src/blockchain_gradido_def.h" "src/utils.h" "src/utils.cpp" "src/dump_blockchain/dump_blockchain.cpp")
target_link_libraries(dump_blockchain -lPocoNet -lPocoUtil -lPocoJSON -lPocoFoundation -lPocoData -lPocoNetSSL -lpthread)

add_executable(dump_group_blockchain "src/blockchain_gradido_def.h" "src/utils.h" "src/utils.cpp" "src/dump_group_blockchain/dump_group_blockchain.cpp")
target_link_libraries(dump_group_blockchain -lPocoNet -lPocoUtil -lPocoJSON -lPocoFoundation -lPocoData -lPocoNetSSL -lpthread)

add_executable(unit_tests ${LOCAL_SRC} ${PROTOS} ${PROTOS_MIRROR} ${PROTOS_GRADIDO} ${UNITTEST_SRC})
target_link_libraries(unit_tests -led25519 -lPocoNet -lPocoUtil -lPocoJSON -lPocoFoundation -lPocoData -lPocoNetSSL -lall -ldl -lgmock -lgtest -lgtest_main -lpthread)

