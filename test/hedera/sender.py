import logging, sys, time, nacl
from nacl.encoding import Base64Encoder, HexEncoder
from nacl.signing import SigningKey

sys.path.append("./proto_gen")


import grpc
from gradido.gradido_pb2_grpc import GradidoNodeServiceStub
from gradido.gradido_pb2 import ManageGroupRequest


#import proto_gen.ConsensusService_pb2_grpc
#import proto_gen.ConsensusService_pb2
#import proto_gen.ConsensusSubmitMessage_pb2

#account_id = "0.0.110582"

# ASN1 ed25519 specification includes some more data inside string,
# provided by hedera; cutting off unnecessary stuff
#private_key = "302e020100300506032b6570042204203933d3e2a2c913a380a2d7fcdb8cf3c1720a9e05f0291cfa473c33c78ad77c4c"[-(32*2):]


def send_some():
    #with grpc.insecure_channel("0.testnet.hedera.com:50211") as channel:
    with grpc.insecure_channel("0.0.0.0:13000") as channel:

        stub = GradidoNodeServiceStub(channel)
        req = ManageGroupRequest(action=0, group="b")

        print dir(req)

        tre = stub.manage_group(request=req)
        print tre
        time.sleep(2) # important


if __name__ == '__main__':
    logging.basicConfig()
    send_some()
