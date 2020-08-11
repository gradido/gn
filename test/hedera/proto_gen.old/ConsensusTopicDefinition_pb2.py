# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: ConsensusTopicDefinition.proto

from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


import BasicTypes_pb2 as BasicTypes__pb2
import Timestamp_pb2 as Timestamp__pb2


DESCRIPTOR = _descriptor.FileDescriptor(
  name='ConsensusTopicDefinition.proto',
  package='proto',
  syntax='proto3',
  serialized_options=b'\n\032com.hedera.hashgraph.protoP\001',
  create_key=_descriptor._internal_create_key,
  serialized_pb=b'\n\x1e\x43onsensusTopicDefinition.proto\x12\x05proto\x1a\x10\x42\x61sicTypes.proto\x1a\x0fTimestamp.proto\"\x8b\x02\n\x18\x43onsensusTopicDefinition\x12\x0c\n\x04memo\x18\x01 \x01(\t\x12(\n\x0evalidStartTime\x18\x02 \x01(\x0b\x32\x10.proto.Timestamp\x12(\n\x0e\x65xpirationTime\x18\x03 \x01(\x0b\x32\x10.proto.Timestamp\x12\x1c\n\x08\x61\x64minKey\x18\x04 \x01(\x0b\x32\n.proto.Key\x12\x1d\n\tsubmitKey\x18\x05 \x01(\x0b\x32\n.proto.Key\x12(\n\x0elastUpdateTime\x18\x06 \x01(\x0b\x32\x10.proto.Timestamp\x12&\n\x0c\x63reationTime\x18\x07 \x01(\x0b\x32\x10.proto.TimestampB\x1e\n\x1a\x63om.hedera.hashgraph.protoP\x01\x62\x06proto3'
  ,
  dependencies=[BasicTypes__pb2.DESCRIPTOR,Timestamp__pb2.DESCRIPTOR,])




_CONSENSUSTOPICDEFINITION = _descriptor.Descriptor(
  name='ConsensusTopicDefinition',
  full_name='proto.ConsensusTopicDefinition',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='memo', full_name='proto.ConsensusTopicDefinition.memo', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='validStartTime', full_name='proto.ConsensusTopicDefinition.validStartTime', index=1,
      number=2, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='expirationTime', full_name='proto.ConsensusTopicDefinition.expirationTime', index=2,
      number=3, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='adminKey', full_name='proto.ConsensusTopicDefinition.adminKey', index=3,
      number=4, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='submitKey', full_name='proto.ConsensusTopicDefinition.submitKey', index=4,
      number=5, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='lastUpdateTime', full_name='proto.ConsensusTopicDefinition.lastUpdateTime', index=5,
      number=6, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='creationTime', full_name='proto.ConsensusTopicDefinition.creationTime', index=6,
      number=7, type=11, cpp_type=10, label=1,
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
  serialized_start=77,
  serialized_end=344,
)

_CONSENSUSTOPICDEFINITION.fields_by_name['validStartTime'].message_type = Timestamp__pb2._TIMESTAMP
_CONSENSUSTOPICDEFINITION.fields_by_name['expirationTime'].message_type = Timestamp__pb2._TIMESTAMP
_CONSENSUSTOPICDEFINITION.fields_by_name['adminKey'].message_type = BasicTypes__pb2._KEY
_CONSENSUSTOPICDEFINITION.fields_by_name['submitKey'].message_type = BasicTypes__pb2._KEY
_CONSENSUSTOPICDEFINITION.fields_by_name['lastUpdateTime'].message_type = Timestamp__pb2._TIMESTAMP
_CONSENSUSTOPICDEFINITION.fields_by_name['creationTime'].message_type = Timestamp__pb2._TIMESTAMP
DESCRIPTOR.message_types_by_name['ConsensusTopicDefinition'] = _CONSENSUSTOPICDEFINITION
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

ConsensusTopicDefinition = _reflection.GeneratedProtocolMessageType('ConsensusTopicDefinition', (_message.Message,), {
  'DESCRIPTOR' : _CONSENSUSTOPICDEFINITION,
  '__module__' : 'ConsensusTopicDefinition_pb2'
  # @@protoc_insertion_point(class_scope:proto.ConsensusTopicDefinition)
  })
_sym_db.RegisterMessage(ConsensusTopicDefinition)


DESCRIPTOR._options = None
# @@protoc_insertion_point(module_scope)
