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
    "build"
)

FILE(GLOB LOCAL_SRC "src/*.cpp" "src/*.c" "src/*.h")
FILE(GLOB PROTOS "proto_gen/*.cc" "proto_gen/*.h")
FILE(GLOB PROTOS_MIRROR "proto_gen/mirror/*.cc" "proto_gen/mirror/*.h")
FILE(GLOB PROTOS_GRADIDO "proto_gen/gradido/*.cc" 
  "proto_gen/gradido/*.h")
FILE(GLOB UNITTEST_SRC "unittest/*.cpp" "unittest/*.h")
FILE(GLOB JSONRPC "src/json_rpc/*.cpp" "src/json_rpc/*.h")

##### libraries
add_library(gradido_proto ${PROTOS} ${PROTOS_MIRROR} ${PROTOS_GRADIDO})
add_library(gradido_base ${LOCAL_SRC} ${JSONRPC})

##### nodes + tools
file(GLOB APP_SOURCES "src/bins/*.cpp")
foreach(sourcefile ${APP_SOURCES})
    file(RELATIVE_PATH filename ${CMAKE_CURRENT_SOURCE_DIR} 
      ${sourcefile})
    get_filename_component(file0 ${sourcefile} NAME)
    string(REPLACE ".cpp" "" file ${file0})
    add_executable(${file} ${filename})
    target_link_libraries(${file} -lgradido_base -lgradido_proto -lmpfr
      -lgmp -led25519 -lPocoNet -lPocoUtil -lPocoJSON -lPocoFoundation
      -lPocoData -lPocoNetSSL -lall -ldl -lpthread)
    add_dependencies(${file} gradido_proto gradido_base)
endforeach(sourcefile ${APP_SOURCES})

##### dump utils 
add_executable(dump_blockchain "src/blockchain_gradido_def.h" 
  "src/utils.h" "src/utils.cpp" 
  "src/dump_blockchain/dump_blockchain.cpp")
target_link_libraries(dump_blockchain -lPocoNet -lPocoUtil -lPocoJSON 
  -lPocoFoundation -lPocoData -lPocoNetSSL -lpthread)

add_executable(dump_group_blockchain "src/blockchain_gradido_def.h" 
  "src/utils.h" "src/utils.cpp" 
  "src/dump_group_blockchain/dump_group_blockchain.cpp")
target_link_libraries(dump_group_blockchain -lPocoNet -lPocoUtil 
  -lPocoJSON -lPocoFoundation -lPocoData -lPocoNetSSL -lpthread)

##### unit tests
add_executable(unit_tests ${LOCAL_SRC} ${JSONRPC} ${UNITTEST_SRC})
target_link_libraries(unit_tests -lgradido_proto -lmpfr -lgmp -led25519
  -lPocoNet -lPocoUtil -lPocoJSON -lPocoFoundation -lPocoData
  -lPocoNetSSL -lall -ldl -lgmock -lgtest -lgtest_main -lpthread)
add_dependencies(unit_tests gradido_proto)


