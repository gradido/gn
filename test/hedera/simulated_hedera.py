from concurrent import futures
import time, sys, threading, copy, os
import math
import logging

import grpc

sys.path.append(os.path.join(os.path.dirname(os.path.abspath(__file__)), "proto_gen"))

from proto_gen.ConsensusService_pb2_grpc import ConsensusServiceServicer
from proto_gen.ConsensusService_pb2_grpc import add_ConsensusServiceServicer_to_server
from proto_gen.TransactionResponse_pb2 import TransactionResponse
from proto_gen.Timestamp_pb2 import Timestamp
from proto_gen.TransactionBody_pb2 import TransactionBody

from proto_gen.mirror.ConsensusService_pb2_grpc import ConsensusServiceServicer as mirror_ConsensusServiceServicer
from proto_gen.mirror.ConsensusService_pb2_grpc import add_ConsensusServiceServicer_to_server as mirror_add_ConsensusServiceServicer_to_server
from proto_gen.mirror.ConsensusService_pb2 import ConsensusTopicQuery
from proto_gen.mirror.ConsensusService_pb2 import ConsensusTopicResponse


class MessageQueue(object):
    def __init__(self):
        self.cond = threading.Condition()
        self.queue = []
    def push(self, msg):
        with self.cond:
            self.queue.append(msg)
            self.cond.notifyAll()
    def pop(self):
        with self.cond:
            if not self.queue:
                self.cond.wait()
            res = self.queue[0]
            self.queue = self.queue[1:]
            return res

class ReservableMessageQueue(MessageQueue):
    def __init__(self):
        MessageQueue.__init__(self)
        self.is_reserved = False


class Topics(object):
    def __init__(self):
        self.topics = {}
        self.lock = threading.Lock()
        self.seq_nums = {}
    def get_ts(self):
        ts = time.time()
        seconds = long(ts)
        nanos = long((ts - seconds) * 1000000000)
        timestamp = Timestamp(seconds=seconds, 
                              nanos=nanos)
        return timestamp
        
    def push(self, topic_id, msg):
        with self.lock:
            if not topic_id in self.topics:
                self.topics[topic_id] = [ReservableMessageQueue()]
            if not topic_id in self.seq_nums:
                self.seq_nums[topic_id] = (1, self.get_ts())
            else:
                (seq_num, _) = self.seq_nums[topic_id]
                self.seq_nums[topic_id] = (seq_num + 1, self.get_ts())
            (seq_num, ts) = self.seq_nums[topic_id]
            for i in self.topics[topic_id]:
                i.push((copy.deepcopy(msg), seq_num, ts))
    def reserve_queue(self, topic_id):
        with self.lock:
            if not topic_id in self.topics:
                self.topics[topic_id] = [ReservableMessageQueue()]
            for i in self.topics[topic_id]:
                if not i.is_reserved:
                    i.is_reserved = True
                    return i
            res = ReservableMessageQueue()
            res.is_reserved = True
            self.topics[topic_id].append(res)
            return res

topics = Topics()            


class ConsensusServiceServicerImpl(ConsensusServiceServicer):
    def submitMessage(self, request, context):
        tb = TransactionBody()
        tb.ParseFromString(request.bodyBytes)
        topic_id = str(tb.consensusSubmitMessage.topicID)
        topics.push(topic_id, request)
        return TransactionResponse(nodeTransactionPrecheckCode=0, 
                                   cost=17)

class mirror_ConsensusServiceServicerImpl(
        mirror_ConsensusServiceServicer):
    def subscribeTopic(self, request, context):
        queue = topics.reserve_queue(str(request.topicID))
        while 1:
            (msg, seq_num, timestamp) = queue.pop()
            sys.stderr.write("seq_num, timestamp: %d, %d.%d\n" % (
                seq_num, timestamp.seconds, timestamp.nanos))

            res = ConsensusTopicResponse(
                consensusTimestamp=timestamp, 
                message=msg.bodyBytes,
                runningHash="\0" * 48,
                sequenceNumber=seq_num,
                runningHashVersion=2
            )
            yield res

def serve():
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    add_ConsensusServiceServicer_to_server(
        ConsensusServiceServicerImpl(), server)
    mirror_add_ConsensusServiceServicer_to_server(
        mirror_ConsensusServiceServicerImpl(), server)

    # TODO: get from config
    server.add_insecure_port('[::]:50051')
    server.start()
    sys.stderr.write("hedera simulator started\n")
    server.wait_for_termination()


if __name__ == '__main__':
    logging.basicConfig()
    serve()
