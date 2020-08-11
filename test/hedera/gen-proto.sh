PROTOC=../../bin/protoc-3.12.2.0
PATH=$PATH:../../bin

mkdir proto_gen

python -m grpc_tools.protoc -I./../../proto --python_out=./proto_gen --grpc_python_out=./proto_gen ./../../proto/mirror/ConsensusService.proto
python -m grpc_tools.protoc -I./../../proto --python_out=./proto_gen --grpc_python_out=./proto_gen ./../../proto/gradido/gradido.proto
python -m grpc_tools.protoc -I./../../proto --python_out=./proto_gen --grpc_python_out=./proto_gen ./../../proto/*.proto

touch proto_gen/__init__.py
touch proto_gen/mirror/__init__.py
touch proto_gen/gradido/__init__.py
