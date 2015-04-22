// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: managerProto.proto

#define INTERNAL_SUPPRESS_PROTOBUF_FIELD_DEPRECATION
#include "managerProto.pb.h"

#include <algorithm>

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/stubs/once.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/generated_message_reflection.h>
#include <google/protobuf/reflection_ops.h>
#include <google/protobuf/wire_format.h>
// @@protoc_insertion_point(includes)

namespace com {
namespace rj {
namespace protos {
namespace manager {

namespace {

const ::google::protobuf::Descriptor* managerProtocol_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  managerProtocol_reflection_ = NULL;
const ::google::protobuf::Descriptor* managerProtocol_messageValue_descriptor_ = NULL;
const ::google::protobuf::internal::GeneratedMessageReflection*
  managerProtocol_messageValue_reflection_ = NULL;
const ::google::protobuf::EnumDescriptor* managerProtocol_messageType_descriptor_ = NULL;
const ::google::protobuf::EnumDescriptor* managerProtocol_messageTrans_descriptor_ = NULL;

}  // namespace


void protobuf_AssignDesc_managerProto_2eproto() {
  protobuf_AddDesc_managerProto_2eproto();
  const ::google::protobuf::FileDescriptor* file =
    ::google::protobuf::DescriptorPool::generated_pool()->FindFileByName(
      "managerProto.proto");
  GOOGLE_CHECK(file != NULL);
  managerProtocol_descriptor_ = file->message_type(0);
  static const int managerProtocol_offsets_[3] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(managerProtocol, messagetype_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(managerProtocol, messagefrom_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(managerProtocol, messagevalue_),
  };
  managerProtocol_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      managerProtocol_descriptor_,
      managerProtocol::default_instance_,
      managerProtocol_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(managerProtocol, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(managerProtocol, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(managerProtocol));
  managerProtocol_messageValue_descriptor_ = managerProtocol_descriptor_->nested_type(0);
  static const int managerProtocol_messageValue_offsets_[5] = {
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(managerProtocol_messageValue, ip_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(managerProtocol_messageValue, port_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(managerProtocol_messageValue, ip1_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(managerProtocol_messageValue, port1_),
    GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(managerProtocol_messageValue, key_),
  };
  managerProtocol_messageValue_reflection_ =
    new ::google::protobuf::internal::GeneratedMessageReflection(
      managerProtocol_messageValue_descriptor_,
      managerProtocol_messageValue::default_instance_,
      managerProtocol_messageValue_offsets_,
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(managerProtocol_messageValue, _has_bits_[0]),
      GOOGLE_PROTOBUF_GENERATED_MESSAGE_FIELD_OFFSET(managerProtocol_messageValue, _unknown_fields_),
      -1,
      ::google::protobuf::DescriptorPool::generated_pool(),
      ::google::protobuf::MessageFactory::generated_factory(),
      sizeof(managerProtocol_messageValue));
  managerProtocol_messageType_descriptor_ = managerProtocol_descriptor_->enum_type(0);
  managerProtocol_messageTrans_descriptor_ = managerProtocol_descriptor_->enum_type(1);
}

namespace {

GOOGLE_PROTOBUF_DECLARE_ONCE(protobuf_AssignDescriptors_once_);
inline void protobuf_AssignDescriptorsOnce() {
  ::google::protobuf::GoogleOnceInit(&protobuf_AssignDescriptors_once_,
                 &protobuf_AssignDesc_managerProto_2eproto);
}

void protobuf_RegisterTypes(const ::std::string&) {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    managerProtocol_descriptor_, &managerProtocol::default_instance());
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedMessage(
    managerProtocol_messageValue_descriptor_, &managerProtocol_messageValue::default_instance());
}

}  // namespace

void protobuf_ShutdownFile_managerProto_2eproto() {
  delete managerProtocol::default_instance_;
  delete managerProtocol_reflection_;
  delete managerProtocol_messageValue::default_instance_;
  delete managerProtocol_messageValue_reflection_;
}

void protobuf_AddDesc_managerProto_2eproto() {
  static bool already_here = false;
  if (already_here) return;
  already_here = true;
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  ::google::protobuf::DescriptorPool::InternalAddGeneratedFile(
    "\n\022managerProto.proto\022\025com.rj.protos.mana"
    "ger\"\362\003\n\017managerProtocol\022G\n\013messagetype\030\001"
    " \002(\01622.com.rj.protos.manager.managerProt"
    "ocol.messageType\022H\n\013messagefrom\030\002 \002(\01623."
    "com.rj.protos.manager.managerProtocol.me"
    "ssageTrans\022I\n\014messagevalue\030\003 \002(\01323.com.r"
    "j.protos.manager.managerProtocol.message"
    "Value\032Q\n\014messageValue\022\n\n\002ip\030\001 \001(\t\022\014\n\004por"
    "t\030\002 \003(\r\022\013\n\003ip1\030\003 \001(\t\022\r\n\005port1\030\004 \001(\r\022\013\n\003k"
    "ey\030\005 \001(\t\"m\n\013messageType\022\r\n\tLOGIN_REQ\020\001\022\r"
    "\n\tLOGIN_RSP\020\002\022\r\n\tHEART_REQ\020\003\022\r\n\tHEART_RS"
    "P\020\004\022\020\n\014REGISTER_REQ\020\005\022\020\n\014REGISTER_RSP\020\006\""
    "\?\n\014messageTrans\022\014\n\010THROTTLE\020\001\022\n\n\006BIDDER\020"
    "\002\022\r\n\tCONNECTOR\020\003\022\006\n\002BC\020\004", 544);
  ::google::protobuf::MessageFactory::InternalRegisterGeneratedFile(
    "managerProto.proto", &protobuf_RegisterTypes);
  managerProtocol::default_instance_ = new managerProtocol();
  managerProtocol_messageValue::default_instance_ = new managerProtocol_messageValue();
  managerProtocol::default_instance_->InitAsDefaultInstance();
  managerProtocol_messageValue::default_instance_->InitAsDefaultInstance();
  ::google::protobuf::internal::OnShutdown(&protobuf_ShutdownFile_managerProto_2eproto);
}

// Force AddDescriptors() to be called at static initialization time.
struct StaticDescriptorInitializer_managerProto_2eproto {
  StaticDescriptorInitializer_managerProto_2eproto() {
    protobuf_AddDesc_managerProto_2eproto();
  }
} static_descriptor_initializer_managerProto_2eproto_;

// ===================================================================

const ::google::protobuf::EnumDescriptor* managerProtocol_messageType_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return managerProtocol_messageType_descriptor_;
}
bool managerProtocol_messageType_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
      return true;
    default:
      return false;
  }
}

#ifndef _MSC_VER
const managerProtocol_messageType managerProtocol::LOGIN_REQ;
const managerProtocol_messageType managerProtocol::LOGIN_RSP;
const managerProtocol_messageType managerProtocol::HEART_REQ;
const managerProtocol_messageType managerProtocol::HEART_RSP;
const managerProtocol_messageType managerProtocol::REGISTER_REQ;
const managerProtocol_messageType managerProtocol::REGISTER_RSP;
const managerProtocol_messageType managerProtocol::messageType_MIN;
const managerProtocol_messageType managerProtocol::messageType_MAX;
const int managerProtocol::messageType_ARRAYSIZE;
#endif  // _MSC_VER
const ::google::protobuf::EnumDescriptor* managerProtocol_messageTrans_descriptor() {
  protobuf_AssignDescriptorsOnce();
  return managerProtocol_messageTrans_descriptor_;
}
bool managerProtocol_messageTrans_IsValid(int value) {
  switch(value) {
    case 1:
    case 2:
    case 3:
    case 4:
      return true;
    default:
      return false;
  }
}

#ifndef _MSC_VER
const managerProtocol_messageTrans managerProtocol::THROTTLE;
const managerProtocol_messageTrans managerProtocol::BIDDER;
const managerProtocol_messageTrans managerProtocol::CONNECTOR;
const managerProtocol_messageTrans managerProtocol::BC;
const managerProtocol_messageTrans managerProtocol::messageTrans_MIN;
const managerProtocol_messageTrans managerProtocol::messageTrans_MAX;
const int managerProtocol::messageTrans_ARRAYSIZE;
#endif  // _MSC_VER
#ifndef _MSC_VER
const int managerProtocol_messageValue::kIpFieldNumber;
const int managerProtocol_messageValue::kPortFieldNumber;
const int managerProtocol_messageValue::kIp1FieldNumber;
const int managerProtocol_messageValue::kPort1FieldNumber;
const int managerProtocol_messageValue::kKeyFieldNumber;
#endif  // !_MSC_VER

managerProtocol_messageValue::managerProtocol_messageValue()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void managerProtocol_messageValue::InitAsDefaultInstance() {
}

managerProtocol_messageValue::managerProtocol_messageValue(const managerProtocol_messageValue& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void managerProtocol_messageValue::SharedCtor() {
  _cached_size_ = 0;
  ip_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
  ip1_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
  port1_ = 0u;
  key_ = const_cast< ::std::string*>(&::google::protobuf::internal::kEmptyString);
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

managerProtocol_messageValue::~managerProtocol_messageValue() {
  SharedDtor();
}

void managerProtocol_messageValue::SharedDtor() {
  if (ip_ != &::google::protobuf::internal::kEmptyString) {
    delete ip_;
  }
  if (ip1_ != &::google::protobuf::internal::kEmptyString) {
    delete ip1_;
  }
  if (key_ != &::google::protobuf::internal::kEmptyString) {
    delete key_;
  }
  if (this != default_instance_) {
  }
}

void managerProtocol_messageValue::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* managerProtocol_messageValue::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return managerProtocol_messageValue_descriptor_;
}

const managerProtocol_messageValue& managerProtocol_messageValue::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_managerProto_2eproto();
  return *default_instance_;
}

managerProtocol_messageValue* managerProtocol_messageValue::default_instance_ = NULL;

managerProtocol_messageValue* managerProtocol_messageValue::New() const {
  return new managerProtocol_messageValue;
}

void managerProtocol_messageValue::Clear() {
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (has_ip()) {
      if (ip_ != &::google::protobuf::internal::kEmptyString) {
        ip_->clear();
      }
    }
    if (has_ip1()) {
      if (ip1_ != &::google::protobuf::internal::kEmptyString) {
        ip1_->clear();
      }
    }
    port1_ = 0u;
    if (has_key()) {
      if (key_ != &::google::protobuf::internal::kEmptyString) {
        key_->clear();
      }
    }
  }
  port_.Clear();
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool managerProtocol_messageValue::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // optional string ip = 1;
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_ip()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8String(
            this->ip().data(), this->ip().length(),
            ::google::protobuf::internal::WireFormat::PARSE);
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(16)) goto parse_port;
        break;
      }

      // repeated uint32 port = 2;
      case 2: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_port:
          DO_((::google::protobuf::internal::WireFormatLite::ReadRepeatedPrimitive<
                   ::google::protobuf::uint32, ::google::protobuf::internal::WireFormatLite::TYPE_UINT32>(
                 1, 16, input, this->mutable_port())));
        } else if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag)
                   == ::google::protobuf::internal::WireFormatLite::
                      WIRETYPE_LENGTH_DELIMITED) {
          DO_((::google::protobuf::internal::WireFormatLite::ReadPackedPrimitiveNoInline<
                   ::google::protobuf::uint32, ::google::protobuf::internal::WireFormatLite::TYPE_UINT32>(
                 input, this->mutable_port())));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(16)) goto parse_port;
        if (input->ExpectTag(26)) goto parse_ip1;
        break;
      }

      // optional string ip1 = 3;
      case 3: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_ip1:
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_ip1()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8String(
            this->ip1().data(), this->ip1().length(),
            ::google::protobuf::internal::WireFormat::PARSE);
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(32)) goto parse_port1;
        break;
      }

      // optional uint32 port1 = 4;
      case 4: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_port1:
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   ::google::protobuf::uint32, ::google::protobuf::internal::WireFormatLite::TYPE_UINT32>(
                 input, &port1_)));
          set_has_port1();
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(42)) goto parse_key;
        break;
      }

      // optional string key = 5;
      case 5: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_key:
          DO_(::google::protobuf::internal::WireFormatLite::ReadString(
                input, this->mutable_key()));
          ::google::protobuf::internal::WireFormat::VerifyUTF8String(
            this->key().data(), this->key().length(),
            ::google::protobuf::internal::WireFormat::PARSE);
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectAtEnd()) return true;
        break;
      }

      default: {
      handle_uninterpreted:
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          return true;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
  return true;
#undef DO_
}

void managerProtocol_messageValue::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // optional string ip = 1;
  if (has_ip()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->ip().data(), this->ip().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    ::google::protobuf::internal::WireFormatLite::WriteString(
      1, this->ip(), output);
  }

  // repeated uint32 port = 2;
  for (int i = 0; i < this->port_size(); i++) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt32(
      2, this->port(i), output);
  }

  // optional string ip1 = 3;
  if (has_ip1()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->ip1().data(), this->ip1().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    ::google::protobuf::internal::WireFormatLite::WriteString(
      3, this->ip1(), output);
  }

  // optional uint32 port1 = 4;
  if (has_port1()) {
    ::google::protobuf::internal::WireFormatLite::WriteUInt32(4, this->port1(), output);
  }

  // optional string key = 5;
  if (has_key()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->key().data(), this->key().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    ::google::protobuf::internal::WireFormatLite::WriteString(
      5, this->key(), output);
  }

  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* managerProtocol_messageValue::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // optional string ip = 1;
  if (has_ip()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->ip().data(), this->ip().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        1, this->ip(), target);
  }

  // repeated uint32 port = 2;
  for (int i = 0; i < this->port_size(); i++) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteUInt32ToArray(2, this->port(i), target);
  }

  // optional string ip1 = 3;
  if (has_ip1()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->ip1().data(), this->ip1().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        3, this->ip1(), target);
  }

  // optional uint32 port1 = 4;
  if (has_port1()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteUInt32ToArray(4, this->port1(), target);
  }

  // optional string key = 5;
  if (has_key()) {
    ::google::protobuf::internal::WireFormat::VerifyUTF8String(
      this->key().data(), this->key().length(),
      ::google::protobuf::internal::WireFormat::SERIALIZE);
    target =
      ::google::protobuf::internal::WireFormatLite::WriteStringToArray(
        5, this->key(), target);
  }

  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int managerProtocol_messageValue::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // optional string ip = 1;
    if (has_ip()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->ip());
    }

    // optional string ip1 = 3;
    if (has_ip1()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->ip1());
    }

    // optional uint32 port1 = 4;
    if (has_port1()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::UInt32Size(
          this->port1());
    }

    // optional string key = 5;
    if (has_key()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::StringSize(
          this->key());
    }

  }
  // repeated uint32 port = 2;
  {
    int data_size = 0;
    for (int i = 0; i < this->port_size(); i++) {
      data_size += ::google::protobuf::internal::WireFormatLite::
        UInt32Size(this->port(i));
    }
    total_size += 1 * this->port_size() + data_size;
  }

  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void managerProtocol_messageValue::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const managerProtocol_messageValue* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const managerProtocol_messageValue*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void managerProtocol_messageValue::MergeFrom(const managerProtocol_messageValue& from) {
  GOOGLE_CHECK_NE(&from, this);
  port_.MergeFrom(from.port_);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_ip()) {
      set_ip(from.ip());
    }
    if (from.has_ip1()) {
      set_ip1(from.ip1());
    }
    if (from.has_port1()) {
      set_port1(from.port1());
    }
    if (from.has_key()) {
      set_key(from.key());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void managerProtocol_messageValue::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void managerProtocol_messageValue::CopyFrom(const managerProtocol_messageValue& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool managerProtocol_messageValue::IsInitialized() const {

  return true;
}

void managerProtocol_messageValue::Swap(managerProtocol_messageValue* other) {
  if (other != this) {
    std::swap(ip_, other->ip_);
    port_.Swap(&other->port_);
    std::swap(ip1_, other->ip1_);
    std::swap(port1_, other->port1_);
    std::swap(key_, other->key_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata managerProtocol_messageValue::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = managerProtocol_messageValue_descriptor_;
  metadata.reflection = managerProtocol_messageValue_reflection_;
  return metadata;
}


// -------------------------------------------------------------------

#ifndef _MSC_VER
const int managerProtocol::kMessagetypeFieldNumber;
const int managerProtocol::kMessagefromFieldNumber;
const int managerProtocol::kMessagevalueFieldNumber;
#endif  // !_MSC_VER

managerProtocol::managerProtocol()
  : ::google::protobuf::Message() {
  SharedCtor();
}

void managerProtocol::InitAsDefaultInstance() {
  messagevalue_ = const_cast< ::com::rj::protos::manager::managerProtocol_messageValue*>(&::com::rj::protos::manager::managerProtocol_messageValue::default_instance());
}

managerProtocol::managerProtocol(const managerProtocol& from)
  : ::google::protobuf::Message() {
  SharedCtor();
  MergeFrom(from);
}

void managerProtocol::SharedCtor() {
  _cached_size_ = 0;
  messagetype_ = 1;
  messagefrom_ = 1;
  messagevalue_ = NULL;
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
}

managerProtocol::~managerProtocol() {
  SharedDtor();
}

void managerProtocol::SharedDtor() {
  if (this != default_instance_) {
    delete messagevalue_;
  }
}

void managerProtocol::SetCachedSize(int size) const {
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
}
const ::google::protobuf::Descriptor* managerProtocol::descriptor() {
  protobuf_AssignDescriptorsOnce();
  return managerProtocol_descriptor_;
}

const managerProtocol& managerProtocol::default_instance() {
  if (default_instance_ == NULL) protobuf_AddDesc_managerProto_2eproto();
  return *default_instance_;
}

managerProtocol* managerProtocol::default_instance_ = NULL;

managerProtocol* managerProtocol::New() const {
  return new managerProtocol;
}

void managerProtocol::Clear() {
  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    messagetype_ = 1;
    messagefrom_ = 1;
    if (has_messagevalue()) {
      if (messagevalue_ != NULL) messagevalue_->::com::rj::protos::manager::managerProtocol_messageValue::Clear();
    }
  }
  ::memset(_has_bits_, 0, sizeof(_has_bits_));
  mutable_unknown_fields()->Clear();
}

bool managerProtocol::MergePartialFromCodedStream(
    ::google::protobuf::io::CodedInputStream* input) {
#define DO_(EXPRESSION) if (!(EXPRESSION)) return false
  ::google::protobuf::uint32 tag;
  while ((tag = input->ReadTag()) != 0) {
    switch (::google::protobuf::internal::WireFormatLite::GetTagFieldNumber(tag)) {
      // required .com.rj.protos.manager.managerProtocol.messageType messagetype = 1;
      case 1: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
          int value;
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   int, ::google::protobuf::internal::WireFormatLite::TYPE_ENUM>(
                 input, &value)));
          if (::com::rj::protos::manager::managerProtocol_messageType_IsValid(value)) {
            set_messagetype(static_cast< ::com::rj::protos::manager::managerProtocol_messageType >(value));
          } else {
            mutable_unknown_fields()->AddVarint(1, value);
          }
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(16)) goto parse_messagefrom;
        break;
      }

      // required .com.rj.protos.manager.managerProtocol.messageTrans messagefrom = 2;
      case 2: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_VARINT) {
         parse_messagefrom:
          int value;
          DO_((::google::protobuf::internal::WireFormatLite::ReadPrimitive<
                   int, ::google::protobuf::internal::WireFormatLite::TYPE_ENUM>(
                 input, &value)));
          if (::com::rj::protos::manager::managerProtocol_messageTrans_IsValid(value)) {
            set_messagefrom(static_cast< ::com::rj::protos::manager::managerProtocol_messageTrans >(value));
          } else {
            mutable_unknown_fields()->AddVarint(2, value);
          }
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectTag(26)) goto parse_messagevalue;
        break;
      }

      // required .com.rj.protos.manager.managerProtocol.messageValue messagevalue = 3;
      case 3: {
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED) {
         parse_messagevalue:
          DO_(::google::protobuf::internal::WireFormatLite::ReadMessageNoVirtual(
               input, mutable_messagevalue()));
        } else {
          goto handle_uninterpreted;
        }
        if (input->ExpectAtEnd()) return true;
        break;
      }

      default: {
      handle_uninterpreted:
        if (::google::protobuf::internal::WireFormatLite::GetTagWireType(tag) ==
            ::google::protobuf::internal::WireFormatLite::WIRETYPE_END_GROUP) {
          return true;
        }
        DO_(::google::protobuf::internal::WireFormat::SkipField(
              input, tag, mutable_unknown_fields()));
        break;
      }
    }
  }
  return true;
#undef DO_
}

void managerProtocol::SerializeWithCachedSizes(
    ::google::protobuf::io::CodedOutputStream* output) const {
  // required .com.rj.protos.manager.managerProtocol.messageType messagetype = 1;
  if (has_messagetype()) {
    ::google::protobuf::internal::WireFormatLite::WriteEnum(
      1, this->messagetype(), output);
  }

  // required .com.rj.protos.manager.managerProtocol.messageTrans messagefrom = 2;
  if (has_messagefrom()) {
    ::google::protobuf::internal::WireFormatLite::WriteEnum(
      2, this->messagefrom(), output);
  }

  // required .com.rj.protos.manager.managerProtocol.messageValue messagevalue = 3;
  if (has_messagevalue()) {
    ::google::protobuf::internal::WireFormatLite::WriteMessageMaybeToArray(
      3, this->messagevalue(), output);
  }

  if (!unknown_fields().empty()) {
    ::google::protobuf::internal::WireFormat::SerializeUnknownFields(
        unknown_fields(), output);
  }
}

::google::protobuf::uint8* managerProtocol::SerializeWithCachedSizesToArray(
    ::google::protobuf::uint8* target) const {
  // required .com.rj.protos.manager.managerProtocol.messageType messagetype = 1;
  if (has_messagetype()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteEnumToArray(
      1, this->messagetype(), target);
  }

  // required .com.rj.protos.manager.managerProtocol.messageTrans messagefrom = 2;
  if (has_messagefrom()) {
    target = ::google::protobuf::internal::WireFormatLite::WriteEnumToArray(
      2, this->messagefrom(), target);
  }

  // required .com.rj.protos.manager.managerProtocol.messageValue messagevalue = 3;
  if (has_messagevalue()) {
    target = ::google::protobuf::internal::WireFormatLite::
      WriteMessageNoVirtualToArray(
        3, this->messagevalue(), target);
  }

  if (!unknown_fields().empty()) {
    target = ::google::protobuf::internal::WireFormat::SerializeUnknownFieldsToArray(
        unknown_fields(), target);
  }
  return target;
}

int managerProtocol::ByteSize() const {
  int total_size = 0;

  if (_has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    // required .com.rj.protos.manager.managerProtocol.messageType messagetype = 1;
    if (has_messagetype()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::EnumSize(this->messagetype());
    }

    // required .com.rj.protos.manager.managerProtocol.messageTrans messagefrom = 2;
    if (has_messagefrom()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::EnumSize(this->messagefrom());
    }

    // required .com.rj.protos.manager.managerProtocol.messageValue messagevalue = 3;
    if (has_messagevalue()) {
      total_size += 1 +
        ::google::protobuf::internal::WireFormatLite::MessageSizeNoVirtual(
          this->messagevalue());
    }

  }
  if (!unknown_fields().empty()) {
    total_size +=
      ::google::protobuf::internal::WireFormat::ComputeUnknownFieldsSize(
        unknown_fields());
  }
  GOOGLE_SAFE_CONCURRENT_WRITES_BEGIN();
  _cached_size_ = total_size;
  GOOGLE_SAFE_CONCURRENT_WRITES_END();
  return total_size;
}

void managerProtocol::MergeFrom(const ::google::protobuf::Message& from) {
  GOOGLE_CHECK_NE(&from, this);
  const managerProtocol* source =
    ::google::protobuf::internal::dynamic_cast_if_available<const managerProtocol*>(
      &from);
  if (source == NULL) {
    ::google::protobuf::internal::ReflectionOps::Merge(from, this);
  } else {
    MergeFrom(*source);
  }
}

void managerProtocol::MergeFrom(const managerProtocol& from) {
  GOOGLE_CHECK_NE(&from, this);
  if (from._has_bits_[0 / 32] & (0xffu << (0 % 32))) {
    if (from.has_messagetype()) {
      set_messagetype(from.messagetype());
    }
    if (from.has_messagefrom()) {
      set_messagefrom(from.messagefrom());
    }
    if (from.has_messagevalue()) {
      mutable_messagevalue()->::com::rj::protos::manager::managerProtocol_messageValue::MergeFrom(from.messagevalue());
    }
  }
  mutable_unknown_fields()->MergeFrom(from.unknown_fields());
}

void managerProtocol::CopyFrom(const ::google::protobuf::Message& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

void managerProtocol::CopyFrom(const managerProtocol& from) {
  if (&from == this) return;
  Clear();
  MergeFrom(from);
}

bool managerProtocol::IsInitialized() const {
  if ((_has_bits_[0] & 0x00000007) != 0x00000007) return false;

  return true;
}

void managerProtocol::Swap(managerProtocol* other) {
  if (other != this) {
    std::swap(messagetype_, other->messagetype_);
    std::swap(messagefrom_, other->messagefrom_);
    std::swap(messagevalue_, other->messagevalue_);
    std::swap(_has_bits_[0], other->_has_bits_[0]);
    _unknown_fields_.Swap(&other->_unknown_fields_);
    std::swap(_cached_size_, other->_cached_size_);
  }
}

::google::protobuf::Metadata managerProtocol::GetMetadata() const {
  protobuf_AssignDescriptorsOnce();
  ::google::protobuf::Metadata metadata;
  metadata.descriptor = managerProtocol_descriptor_;
  metadata.reflection = managerProtocol_reflection_;
  return metadata;
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace manager
}  // namespace protos
}  // namespace rj
}  // namespace com

// @@protoc_insertion_point(global_scope)
