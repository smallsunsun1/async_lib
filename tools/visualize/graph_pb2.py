# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: async/runtime/proto/graph.proto
"""Generated protocol buffer code."""
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


import importlib
async_dot_runtime_dot_proto_dot_node__pb2 = importlib.import_module('async.runtime.proto.node_pb2')


DESCRIPTOR = _descriptor.FileDescriptor(
  name='async/runtime/proto/graph.proto',
  package='sss.async',
  syntax='proto3',
  serialized_options=b'\370\001\001',
  create_key=_descriptor._internal_create_key,
  serialized_pb=b'\n\x1f\x61sync/runtime/proto/graph.proto\x12\tsss.async\x1a\x1e\x61sync/runtime/proto/node.proto\"H\n\rAsyncGraphDef\x12&\n\x05nodes\x18\x01 \x03(\x0b\x32\x17.sss.async.AsyncNodeDef\x12\x0f\n\x07version\x18\x02 \x01(\tB\x03\xf8\x01\x01\x62\x06proto3'
  ,
  dependencies=[async_dot_runtime_dot_proto_dot_node__pb2.DESCRIPTOR,])




_ASYNCGRAPHDEF = _descriptor.Descriptor(
  name='AsyncGraphDef',
  full_name='sss.async.AsyncGraphDef',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='nodes', full_name='sss.async.AsyncGraphDef.nodes', index=0,
      number=1, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='version', full_name='sss.async.AsyncGraphDef.version', index=1,
      number=2, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
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
  serialized_start=78,
  serialized_end=150,
)

_ASYNCGRAPHDEF.fields_by_name['nodes'].message_type = async_dot_runtime_dot_proto_dot_node__pb2._ASYNCNODEDEF
DESCRIPTOR.message_types_by_name['AsyncGraphDef'] = _ASYNCGRAPHDEF
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

AsyncGraphDef = _reflection.GeneratedProtocolMessageType('AsyncGraphDef', (_message.Message,), {
  'DESCRIPTOR' : _ASYNCGRAPHDEF,
  '__module__' : 'async.runtime.proto.graph_pb2'
  # @@protoc_insertion_point(class_scope:sss.async.AsyncGraphDef)
  })
_sym_db.RegisterMessage(AsyncGraphDef)


DESCRIPTOR._options = None
# @@protoc_insertion_point(module_scope)