# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: CryptoTransfer.proto

from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


import BasicTypes_pb2 as BasicTypes__pb2


DESCRIPTOR = _descriptor.FileDescriptor(
  name='CryptoTransfer.proto',
  package='proto',
  syntax='proto3',
  serialized_options=b'\n\032com.hedera.hashgraph.protoP\001',
  create_key=_descriptor._internal_create_key,
  serialized_pb=b'\n\x14\x43ryptoTransfer.proto\x12\x05proto\x1a\x10\x42\x61sicTypes.proto\"D\n\rAccountAmount\x12#\n\taccountID\x18\x01 \x01(\x0b\x32\x10.proto.AccountID\x12\x0e\n\x06\x61mount\x18\x02 \x01(\x12\"<\n\x0cTransferList\x12,\n\x0e\x61\x63\x63ountAmounts\x18\x01 \x03(\x0b\x32\x14.proto.AccountAmount\"G\n\x1d\x43ryptoTransferTransactionBody\x12&\n\ttransfers\x18\x01 \x01(\x0b\x32\x13.proto.TransferListB\x1e\n\x1a\x63om.hedera.hashgraph.protoP\x01\x62\x06proto3'
  ,
  dependencies=[BasicTypes__pb2.DESCRIPTOR,])




_ACCOUNTAMOUNT = _descriptor.Descriptor(
  name='AccountAmount',
  full_name='proto.AccountAmount',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='accountID', full_name='proto.AccountAmount.accountID', index=0,
      number=1, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='amount', full_name='proto.AccountAmount.amount', index=1,
      number=2, type=18, cpp_type=2, label=1,
      has_default_value=False, default_value=0,
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
  serialized_start=49,
  serialized_end=117,
)


_TRANSFERLIST = _descriptor.Descriptor(
  name='TransferList',
  full_name='proto.TransferList',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='accountAmounts', full_name='proto.TransferList.accountAmounts', index=0,
      number=1, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
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
  serialized_start=119,
  serialized_end=179,
)


_CRYPTOTRANSFERTRANSACTIONBODY = _descriptor.Descriptor(
  name='CryptoTransferTransactionBody',
  full_name='proto.CryptoTransferTransactionBody',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='transfers', full_name='proto.CryptoTransferTransactionBody.transfers', index=0,
      number=1, type=11, cpp_type=10, label=1,
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
  serialized_start=181,
  serialized_end=252,
)

_ACCOUNTAMOUNT.fields_by_name['accountID'].message_type = BasicTypes__pb2._ACCOUNTID
_TRANSFERLIST.fields_by_name['accountAmounts'].message_type = _ACCOUNTAMOUNT
_CRYPTOTRANSFERTRANSACTIONBODY.fields_by_name['transfers'].message_type = _TRANSFERLIST
DESCRIPTOR.message_types_by_name['AccountAmount'] = _ACCOUNTAMOUNT
DESCRIPTOR.message_types_by_name['TransferList'] = _TRANSFERLIST
DESCRIPTOR.message_types_by_name['CryptoTransferTransactionBody'] = _CRYPTOTRANSFERTRANSACTIONBODY
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

AccountAmount = _reflection.GeneratedProtocolMessageType('AccountAmount', (_message.Message,), {
  'DESCRIPTOR' : _ACCOUNTAMOUNT,
  '__module__' : 'CryptoTransfer_pb2'
  # @@protoc_insertion_point(class_scope:proto.AccountAmount)
  })
_sym_db.RegisterMessage(AccountAmount)

TransferList = _reflection.GeneratedProtocolMessageType('TransferList', (_message.Message,), {
  'DESCRIPTOR' : _TRANSFERLIST,
  '__module__' : 'CryptoTransfer_pb2'
  # @@protoc_insertion_point(class_scope:proto.TransferList)
  })
_sym_db.RegisterMessage(TransferList)

CryptoTransferTransactionBody = _reflection.GeneratedProtocolMessageType('CryptoTransferTransactionBody', (_message.Message,), {
  'DESCRIPTOR' : _CRYPTOTRANSFERTRANSACTIONBODY,
  '__module__' : 'CryptoTransfer_pb2'
  # @@protoc_insertion_point(class_scope:proto.CryptoTransferTransactionBody)
  })
_sym_db.RegisterMessage(CryptoTransferTransactionBody)


DESCRIPTOR._options = None
# @@protoc_insertion_point(module_scope)
