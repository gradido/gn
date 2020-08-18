PROTOC=`pwd`/bin/protoc-3.12.2.0
BIN=`pwd`/bin

mkdir proto_gen -p

# TODO: check if those three are needed
${PROTOC} -I./proto --cpp_out=./proto_gen ./proto/mirror/ConsensusService.proto
${PROTOC} -I./proto --cpp_out=./proto_gen ./proto/*.proto
${PROTOC} -I./proto --cpp_out=./proto_gen ./proto/gradido/gradido.proto

${PROTOC} -I./proto --grpc_out=./proto_gen --plugin=protoc-gen-grpc=${BIN}/grpc_cpp_plugin ./proto/gradido/gradido.proto
${PROTOC} -I./proto --grpc_out=./proto_gen --plugin=protoc-gen-grpc=${BIN}/grpc_cpp_plugin ./proto/mirror/ConsensusService.proto
${PROTOC} -I./proto --grpc_out=./proto_gen --plugin=protoc-gen-grpc=${BIN}/grpc_cpp_plugin ./proto/*.proto
