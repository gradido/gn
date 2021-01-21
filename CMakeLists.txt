cmake_minimum_required(VERSION 3.4)
project(Gradido_Node C CXX)
add_definitions("-pthread" "-g")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)

execute_process(
  COMMAND git rev-parse HEAD 
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  OUTPUT_VARIABLE git_hash)
add_definitions(-DGIT_HASH_VERSION=${git_hash})
# protoc generated header files cause some warnings about deprecation
add_compile_options(-Wno-deprecated-declarations)
set(CMAKE_CXX_COMPILER_LAUNCHER ccache)

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
FILE(GLOB GRADIDO_MATH "src/gradidomath/*.c" "src/gradidomath/*.h")
FILE(GLOB PROTOS "proto_gen/*.cc" "proto_gen/*.h")
FILE(GLOB PROTOS_MIRROR "proto_gen/mirror/*.cc" "proto_gen/mirror/*.h")
FILE(GLOB PROTOS_GRADIDO "proto_gen/gradido/*.cc" 
  "proto_gen/gradido/*.h")
FILE(GLOB UNITTEST_SRC "unittest/*.cpp" "unittest/*.h")
FILE(GLOB JSONRPC "src/json_rpc/*.cpp" "src/json_rpc/*.h")

##### libraries
add_library(gradido_math ${GRADIDO_MATH})
add_library(gradido_proto ${PROTOS} ${PROTOS_MIRROR} ${PROTOS_GRADIDO})
# disabling all warnings as proto file compilation produces them in large
# numbers
target_compile_options(gradido_proto PRIVATE -w)

add_library(gradido_base ${LOCAL_SRC} ${JSONRPC})

##### nodes + tools
file(GLOB APP_SOURCES "src/bins/*.cpp")
foreach(sourcefile ${APP_SOURCES})
    file(RELATIVE_PATH filename ${CMAKE_CURRENT_SOURCE_DIR} 
      ${sourcefile})
    get_filename_component(file0 ${sourcefile} NAME)
    string(REPLACE ".cpp" "" file ${file0})
    add_executable(${file} ${filename})
    target_link_libraries(${file} 
      ${CMAKE_CURRENT_SOURCE_DIR}/build/libgradido_base.a 
      ${CMAKE_CURRENT_SOURCE_DIR}/build/libgradido_proto.a
      ${CMAKE_CURRENT_SOURCE_DIR}/build/libgradido_math.a)
    target_link_libraries(${file} -lmpfr -lgmp -led25519 -lPocoNet 
      -lPocoUtil -lPocoJSON -lPocoFoundation -lPocoData -lPocoNetSSL 
      -lall -ldl -lpthread)
    add_dependencies(${file} gradido_proto gradido_base gradido_math)
endforeach(sourcefile ${APP_SOURCES})

##### unit tests
add_executable(unit_tests ${LOCAL_SRC} ${JSONRPC} ${UNITTEST_SRC})
target_link_libraries(unit_tests
  ${CMAKE_CURRENT_SOURCE_DIR}/build/libgradido_base.a 
  ${CMAKE_CURRENT_SOURCE_DIR}/build/libgradido_proto.a
  ${CMAKE_CURRENT_SOURCE_DIR}/build/libgradido_math.a)
target_link_libraries(unit_tests -lmpfr -lgmp -led25519
  -lPocoNet -lPocoUtil -lPocoJSON -lPocoFoundation -lPocoData
  -lPocoNetSSL -lall -ldl -lgmock -lgtest -lgtest_main -lgradido_math 
  -lpthread)
add_dependencies(unit_tests gradido_proto gradido_base gradido_math)

