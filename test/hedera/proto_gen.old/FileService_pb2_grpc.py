# Generated by the gRPC Python protocol compiler plugin. DO NOT EDIT!
"""Client and server classes corresponding to protobuf-defined services."""
import grpc

import Query_pb2 as Query__pb2
import Response_pb2 as Response__pb2
import TransactionResponse_pb2 as TransactionResponse__pb2
import Transaction_pb2 as Transaction__pb2


class FileServiceStub(object):
    """
    Transactions and queries for the file service.
    """

    def __init__(self, channel):
        """Constructor.

        Args:
            channel: A grpc.Channel.
        """
        self.createFile = channel.unary_unary(
                '/proto.FileService/createFile',
                request_serializer=Transaction__pb2.Transaction.SerializeToString,
                response_deserializer=TransactionResponse__pb2.TransactionResponse.FromString,
                )
        self.updateFile = channel.unary_unary(
                '/proto.FileService/updateFile',
                request_serializer=Transaction__pb2.Transaction.SerializeToString,
                response_deserializer=TransactionResponse__pb2.TransactionResponse.FromString,
                )
        self.deleteFile = channel.unary_unary(
                '/proto.FileService/deleteFile',
                request_serializer=Transaction__pb2.Transaction.SerializeToString,
                response_deserializer=TransactionResponse__pb2.TransactionResponse.FromString,
                )
        self.appendContent = channel.unary_unary(
                '/proto.FileService/appendContent',
                request_serializer=Transaction__pb2.Transaction.SerializeToString,
                response_deserializer=TransactionResponse__pb2.TransactionResponse.FromString,
                )
        self.getFileContent = channel.unary_unary(
                '/proto.FileService/getFileContent',
                request_serializer=Query__pb2.Query.SerializeToString,
                response_deserializer=Response__pb2.Response.FromString,
                )
        self.getFileInfo = channel.unary_unary(
                '/proto.FileService/getFileInfo',
                request_serializer=Query__pb2.Query.SerializeToString,
                response_deserializer=Response__pb2.Response.FromString,
                )
        self.systemDelete = channel.unary_unary(
                '/proto.FileService/systemDelete',
                request_serializer=Transaction__pb2.Transaction.SerializeToString,
                response_deserializer=TransactionResponse__pb2.TransactionResponse.FromString,
                )
        self.systemUndelete = channel.unary_unary(
                '/proto.FileService/systemUndelete',
                request_serializer=Transaction__pb2.Transaction.SerializeToString,
                response_deserializer=TransactionResponse__pb2.TransactionResponse.FromString,
                )


class FileServiceServicer(object):
    """
    Transactions and queries for the file service.
    """

    def createFile(self, request, context):
        """Creates a file
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def updateFile(self, request, context):
        """Updates a file
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def deleteFile(self, request, context):
        """Deletes a file
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def appendContent(self, request, context):
        """Appends to a file
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def getFileContent(self, request, context):
        """Retrieves the file contents
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def getFileInfo(self, request, context):
        """Retrieves the file information
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def systemDelete(self, request, context):
        """Deletes a file if the submitting account has network admin privileges
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')

    def systemUndelete(self, request, context):
        """Undeletes a file if the submitting account has network admin privileges
        """
        context.set_code(grpc.StatusCode.UNIMPLEMENTED)
        context.set_details('Method not implemented!')
        raise NotImplementedError('Method not implemented!')


def add_FileServiceServicer_to_server(servicer, server):
    rpc_method_handlers = {
            'createFile': grpc.unary_unary_rpc_method_handler(
                    servicer.createFile,
                    request_deserializer=Transaction__pb2.Transaction.FromString,
                    response_serializer=TransactionResponse__pb2.TransactionResponse.SerializeToString,
            ),
            'updateFile': grpc.unary_unary_rpc_method_handler(
                    servicer.updateFile,
                    request_deserializer=Transaction__pb2.Transaction.FromString,
                    response_serializer=TransactionResponse__pb2.TransactionResponse.SerializeToString,
            ),
            'deleteFile': grpc.unary_unary_rpc_method_handler(
                    servicer.deleteFile,
                    request_deserializer=Transaction__pb2.Transaction.FromString,
                    response_serializer=TransactionResponse__pb2.TransactionResponse.SerializeToString,
            ),
            'appendContent': grpc.unary_unary_rpc_method_handler(
                    servicer.appendContent,
                    request_deserializer=Transaction__pb2.Transaction.FromString,
                    response_serializer=TransactionResponse__pb2.TransactionResponse.SerializeToString,
            ),
            'getFileContent': grpc.unary_unary_rpc_method_handler(
                    servicer.getFileContent,
                    request_deserializer=Query__pb2.Query.FromString,
                    response_serializer=Response__pb2.Response.SerializeToString,
            ),
            'getFileInfo': grpc.unary_unary_rpc_method_handler(
                    servicer.getFileInfo,
                    request_deserializer=Query__pb2.Query.FromString,
                    response_serializer=Response__pb2.Response.SerializeToString,
            ),
            'systemDelete': grpc.unary_unary_rpc_method_handler(
                    servicer.systemDelete,
                    request_deserializer=Transaction__pb2.Transaction.FromString,
                    response_serializer=TransactionResponse__pb2.TransactionResponse.SerializeToString,
            ),
            'systemUndelete': grpc.unary_unary_rpc_method_handler(
                    servicer.systemUndelete,
                    request_deserializer=Transaction__pb2.Transaction.FromString,
                    response_serializer=TransactionResponse__pb2.TransactionResponse.SerializeToString,
            ),
    }
    generic_handler = grpc.method_handlers_generic_handler(
            'proto.FileService', rpc_method_handlers)
    server.add_generic_rpc_handlers((generic_handler,))


 # This class is part of an EXPERIMENTAL API.
class FileService(object):
    """
    Transactions and queries for the file service.
    """

    @staticmethod
    def createFile(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/proto.FileService/createFile',
            Transaction__pb2.Transaction.SerializeToString,
            TransactionResponse__pb2.TransactionResponse.FromString,
            options, channel_credentials,
            call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def updateFile(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/proto.FileService/updateFile',
            Transaction__pb2.Transaction.SerializeToString,
            TransactionResponse__pb2.TransactionResponse.FromString,
            options, channel_credentials,
            call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def deleteFile(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/proto.FileService/deleteFile',
            Transaction__pb2.Transaction.SerializeToString,
            TransactionResponse__pb2.TransactionResponse.FromString,
            options, channel_credentials,
            call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def appendContent(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/proto.FileService/appendContent',
            Transaction__pb2.Transaction.SerializeToString,
            TransactionResponse__pb2.TransactionResponse.FromString,
            options, channel_credentials,
            call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def getFileContent(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/proto.FileService/getFileContent',
            Query__pb2.Query.SerializeToString,
            Response__pb2.Response.FromString,
            options, channel_credentials,
            call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def getFileInfo(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/proto.FileService/getFileInfo',
            Query__pb2.Query.SerializeToString,
            Response__pb2.Response.FromString,
            options, channel_credentials,
            call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def systemDelete(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/proto.FileService/systemDelete',
            Transaction__pb2.Transaction.SerializeToString,
            TransactionResponse__pb2.TransactionResponse.FromString,
            options, channel_credentials,
            call_credentials, compression, wait_for_ready, timeout, metadata)

    @staticmethod
    def systemUndelete(request,
            target,
            options=(),
            channel_credentials=None,
            call_credentials=None,
            compression=None,
            wait_for_ready=None,
            timeout=None,
            metadata=None):
        return grpc.experimental.unary_unary(request, target, '/proto.FileService/systemUndelete',
            Transaction__pb2.Transaction.SerializeToString,
            TransactionResponse__pb2.TransactionResponse.FromString,
            options, channel_credentials,
            call_credentials, compression, wait_for_ready, timeout, metadata)
