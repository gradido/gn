# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: CryptoCreate.proto

from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


import BasicTypes_pb2 as BasicTypes__pb2
import Duration_pb2 as Duration__pb2


DESCRIPTOR = _descriptor.FileDescriptor(
  name='CryptoCreate.proto',
  package='proto',
  syntax='proto3',
  serialized_options=b'\n\032com.hedera.hashgraph.protoP\001',
  create_key=_descriptor._internal_create_key,
  serialized_pb=b'\n\x12\x43ryptoCreate.proto\x12\x05proto\x1a\x10\x42\x61sicTypes.proto\x1a\x0e\x44uration.proto\"\xe4\x02\n\x1b\x43ryptoCreateTransactionBody\x12\x17\n\x03key\x18\x01 \x01(\x0b\x32\n.proto.Key\x12\x16\n\x0einitialBalance\x18\x02 \x01(\x04\x12(\n\x0eproxyAccountID\x18\x03 \x01(\x0b\x32\x10.proto.AccountID\x12\x1b\n\x13sendRecordThreshold\x18\x06 \x01(\x04\x12\x1e\n\x16receiveRecordThreshold\x18\x07 \x01(\x04\x12\x1b\n\x13receiverSigRequired\x18\x08 \x01(\x08\x12(\n\x0f\x61utoRenewPeriod\x18\t \x01(\x0b\x32\x0f.proto.Duration\x12\x1f\n\x07shardID\x18\n \x01(\x0b\x32\x0e.proto.ShardID\x12\x1f\n\x07realmID\x18\x0b \x01(\x0b\x32\x0e.proto.RealmID\x12$\n\x10newRealmAdminKey\x18\x0c \x01(\x0b\x32\n.proto.KeyB\x1e\n\x1a\x63om.hedera.hashgraph.protoP\x01\x62\x06proto3'
  ,
  dependencies=[BasicTypes__pb2.DESCRIPTOR,Duration__pb2.DESCRIPTOR,])




_CRYPTOCREATETRANSACTIONBODY = _descriptor.Descriptor(
  name='CryptoCreateTransactionBody',
  full_name='proto.CryptoCreateTransactionBody',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='key', full_name='proto.CryptoCreateTransactionBody.key', index=0,
      number=1, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='initialBalance', full_name='proto.CryptoCreateTransactionBody.initialBalance', index=1,
      number=2, type=4, cpp_type=4, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='proxyAccountID', full_name='proto.CryptoCreateTransactionBody.proxyAccountID', index=2,
      number=3, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='sendRecordThreshold', full_name='proto.CryptoCreateTransactionBody.sendRecordThreshold', index=3,
      number=6, type=4, cpp_type=4, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='receiveRecordThreshold', full_name='proto.CryptoCreateTransactionBody.receiveRecordThreshold', index=4,
      number=7, type=4, cpp_type=4, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='receiverSigRequired', full_name='proto.CryptoCreateTransactionBody.receiverSigRequired', index=5,
      number=8, type=8, cpp_type=7, label=1,
      has_default_value=False, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='autoRenewPeriod', full_name='proto.CryptoCreateTransactionBody.autoRenewPeriod', index=6,
      number=9, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='shardID', full_name='proto.CryptoCreateTransactionBody.shardID', index=7,
      number=10, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='realmID', full_name='proto.CryptoCreateTransactionBody.realmID', index=8,
      number=11, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='newRealmAdminKey', full_name='proto.CryptoCreateTransactionBody.newRealmAdminKey', index=9,
      number=12, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=64,
  serialized_end=420,
)

_CRYPTOCREATETRANSACTIONBODY.fields_by_name['key'].message_type = BasicTypes__pb2._KEY
_CRYPTOCREATETRANSACTIONBODY.fields_by_name['proxyAccountID'].message_type = BasicTypes__pb2._ACCOUNTID
_CRYPTOCREATETRANSACTIONBODY.fields_by_name['autoRenewPeriod'].message_type = Duration__pb2._DURATION
_CRYPTOCREATETRANSACTIONBODY.fields_by_name['shardID'].message_type = BasicTypes__pb2._SHARDID
_CRYPTOCREATETRANSACTIONBODY.fields_by_name['realmID'].message_type = BasicTypes__pb2._REALMID
_CRYPTOCREATETRANSACTIONBODY.fields_by_name['newRealmAdminKey'].message_type = BasicTypes__pb2._KEY
DESCRIPTOR.message_types_by_name['CryptoCreateTransactionBody'] = _CRYPTOCREATETRANSACTIONBODY
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

CryptoCreateTransactionBody = _reflection.GeneratedProtocolMessageType('CryptoCreateTransactionBody', (_message.Message,), {
  'DESCRIPTOR' : _CRYPTOCREATETRANSACTIONBODY,
  '__module__' : 'CryptoCreate_pb2'
  # @@protoc_insertion_point(class_scope:proto.CryptoCreateTransactionBody)
  })
_sym_db.RegisterMessage(CryptoCreateTransactionBody)


DESCRIPTOR._options = None
# @@protoc_insertion_point(module_scope)
