# Generated by the gRPC Python protocol compiler plugin. DO NOT EDIT!
"""Client and server classes corresponding to protobuf-defined services."""
import grpc

import Query_pb2 as Query__pb2
import Response_pb2 as Response__pb2


class NetworkServiceStub(object):
    """The requests and responses for different network services. 
    """

    def __init__(self, channel):
        """Constructor.

        Args:
            channel: A grpc.Channel.
        """
        self.getVersionInfo = channel.unary_unary(
                '/proto.NetworkService/getVersionInfo',
                request_serializer=Query__pb2.Query.SerializeToString,
                response_deserializer=Response__pb2.Response.FromString,
                )


class NetworkServiceServicer(object):
    """The requests and responses for different network services. 
    """

    def getVersionInfo(self, request, context):
        """Retrieves the active versions of Hedera Services and HAPI proto
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')


def add_NetworkServiceServicer_to_server(servicer, server):
    rpc_method_handlers = {
            'getVersionInfo': grpc.unary_unary_rpc_method_handler(
                    servicer.getVersionInfo,
                    request_deserializer=Query__pb2.Query.FromString,
                    response_serializer=Response__pb2.Response.SerializeToString,
            ),
    }
    generic_handler = grpc.method_handlers_generic_handler(
            'proto.NetworkService', rpc_method_handlers)
    server.add_generic_rpc_handlers((generic_handler,))


 # This class is part of an EXPERIMENTAL API.
class NetworkService(object):
    """The requests and responses for different network services. 
    """

    @staticmethod
    def getVersionInfo(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/proto.NetworkService/getVersionInfo',
            Query__pb2.Query.SerializeToString,
            Response__pb2.Response.FromString,
            options, channel_credentials,
            call_credentials, compression, wait_for_ready, timeout, metadata)
