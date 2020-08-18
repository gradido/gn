import logging, sys, time, nacl, configparser, copy
from nacl.encoding import Base64Encoder, HexEncoder
from nacl.signing import SigningKey
from pukala import Path

import grpc

from proto_gen.mirror.ConsensusService_pb2_grpc import ConsensusServiceStub as ConsensusServiceStub_mirror
from proto_gen.BasicTypes_pb2 import AccountID, TransactionID, SignaturePair, SignatureMap, TopicID

from proto_gen.ConsensusService_pb2_grpc import ConsensusServiceStub
from proto_gen.Transaction_pb2 import Transaction
from proto_gen.ConsensusCreateTopic_pb2 import ConsensusCreateTopicTransactionBody
from proto_gen.TransactionBody_pb2 import TransactionBody
from proto_gen.Duration_pb2 import Duration
from proto_gen.Timestamp_pb2 import Timestamp
from proto_gen.TransactionGetReceipt_pb2 import TransactionGetReceiptQuery
from proto_gen.Query_pb2 import Query
from proto_gen.QueryHeader_pb2 import QueryHeader
from proto_gen.CryptoService_pb2_grpc import CryptoServiceStub
from proto_gen.ConsensusSubmitMessage_pb2 import ConsensusSubmitMessageTransactionBody

from proto_gen.gradido.gradido_pb2 import Transaction as Transaction_gradido
from proto_gen.gradido.gradido_pb2 import GradidoTransaction
from proto_gen.gradido.gradido_pb2 import Participant
from proto_gen.gradido.gradido_pb2 import LocalTransfer

# TODO: remove
class Utils(object):
    def __init__(self, channel):
        self.channel = channel
    def sleep(self, settle_time):
        print "sleeping for %d seconds" % settle_time
        time.sleep(settle_time)
        return "slept for %d" % settle_time

class Signer(object):
    def __init__(self, context, body_path):
        self.sk = nacl.signing.SigningKey(HexEncoder.decode(context.private_key))
        self.signed = str(self.sk.sign(context.get_transaction_part(body_path)))

class SomeTransactionID(object):
    def __init__(self, context, transaction_path):
        self.grpc_obj = context.get_transaction_part(transaction_path)
    def get_grpc_obj(self):
        return self.grpc_obj

class PubKeyWrapper(Signer):
    def __init__(self, context, body_path):
        Signer.__init__(self, context, body_path)
    def __str__(self):
        return str(self.sk.verify_key)

class ed25519Signature(Signer):
    def __init__(self, context, body_path):
        Signer.__init__(self, context, body_path)
    def __str__(self):
        return self.signed
    

class HederaContext(object):
    def get_transaction_part(self, body_path):
        rp = Path(body_path, self.transaction_args, [])
        return rp.get_val(self.transaction_args)
    
    def get_hedera_account_id(self, context):
        hcf = context.doc["test-config"]["hedera-credentials-file"]
        if hcf:
            config = configparser.ConfigParser()
            config.read(context.doc["test-config"]["hedera-credentials-file"])
            self.private_key = config['default']["private_key"]
            self.account_id = [int(i) for i in config['default']["account_id"].split(".")]
        else:
            self.private_key = None
            self.account_id = None
        return self.account_id

    def gather_kw_list(self, arg, res):
        for i in arg:
            val = None
            if isinstance(i, dict):
                val = {}
                self.gather_kw(i, val)
            elif isinstance(i, list):
                val = []
                self.gather_kw_list(i, val)
            else:
                val = i
            res.append(val)

    def gather_kw(self, arg, res):
        for i in arg:
            if i.startswith("_"):
                continue
            val = None
            if isinstance(arg[i], dict):
                if "_transformed_use_with" in arg[i]:
                    val = arg[i]["_transformed_use_with"]
                else:
                    val = {}
                    self.gather_kw(arg[i], val)
            elif isinstance(arg[i], list):
                val = []
                self.gather_kw_list(arg[i], val)
            else:
                val = arg[i]
            res[i] = val

    def set_use_with(self, arg):
        if "_use_with" in arg:
            uw = arg["_use_with"]
            if uw == "str":
                arg["_transformed_use_with"] = str(arg["_transformed"])
            else:
                arg["_transformed_use_with"] = getattr(arg["_transformed"], uw)()
        else:
            arg["_transformed_use_with"] = arg["_transformed"]
        

    def transform_into_grpc(self, arg, pass_num):
        if isinstance(arg, dict):
            if "_transformed" in arg:
                return True
            elif not "_type" in arg:
                res = True
                for i in arg:
                    if i.startswith("_"):
                        continue
                    res = res and self.transform_into_grpc(arg[i], pass_num)
                return res
            elif arg["_type"] == "HederaContext":
                arg["_transformed"] = self
                arg["_transformed_use_with"] = arg["_transformed"]
                return True
            elif "_pass" in arg and arg["_pass"] > pass_num:
                return False
            else:
                res = True
                for i in arg:
                    if i.startswith("_"):
                        continue
                    res = res and self.transform_into_grpc(arg[i], pass_num)
                if res:
                    cls = globals()[arg["_type"]]
                    kw = {}
                    self.gather_kw(arg, kw)
                    arg["_transformed"] = cls(**kw)
                    self.set_use_with(arg)
                return res
        elif isinstance(arg, list):
            res = True
            for i in arg:
                res = res and self.transform_into_grpc(i, pass_num)
            return res
        else:
            return True

    def extract_response(self, r):
        return "%s, %s" % (str(r.nodeTransactionPrecheckCode), str(r))

    def do_hedera_calls(self, context):
        ii = context.path.as_arr()[1]
        inst = context.doc["steps"][ii]
        inp = copy.deepcopy(inst["input"]["val"])
        self.transaction_args = []
        res = []

        with grpc.insecure_channel(context.doc["test-config"]["hedera-node-endpoint"]) as channel:
            for i in inp:
                stub = globals()[i["_class"]](channel)
                method = i["_method"]
                arg = i["_arg"]

                self.transaction_args.append(arg)
                j = 0
                while not self.transform_into_grpc(arg, j) and j < 100:
                    j += 1
                if j == 100:
                    raise Exception("pass limit exceeded at step %d" % ii)
                kw = {}
                self.gather_kw(arg, kw)
                res.append(getattr(stub, method)(**kw))
        # should extract data explicitly with _output: field_name: method_to_call
        return [self.extract_response(i) for i in res]
                
                
