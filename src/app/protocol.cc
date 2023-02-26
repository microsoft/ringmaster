#include <stdexcept>
#include "protocol.hh"
#include "serialization.hh"

using namespace std;

Datagram::Datagram(const uint32_t _frame_id,
                   const FrameType _frame_type,
                   const uint16_t _frag_id,
                   const uint16_t _frag_cnt,
                   const string_view _payload)
  : frame_id(_frame_id), frame_type(_frame_type),
    frag_id(_frag_id), frag_cnt(_frag_cnt), payload(_payload)
{}

size_t Datagram::max_payload = 1500 - 28 - Datagram::HEADER_SIZE;

void Datagram::set_mtu(const size_t mtu)
{
  if (mtu > 1500 or mtu < 512) {
    throw runtime_error("reasonable MTU is between 512 and 1500 bytes");
  }

  // MTU - (IP + UDP headers) - Datagram header
  max_payload = mtu - 28 - Datagram::HEADER_SIZE;
}

bool Datagram::parse_from_string(const string & binary)
{
  if (binary.size() < HEADER_SIZE) {
    return false; // datagram is too small to contain a header
  }

  WireParser parser(binary);
  frame_id = parser.read_uint32();
  frame_type = static_cast<FrameType>(parser.read_uint8());
  frag_id = parser.read_uint16();
  frag_cnt = parser.read_uint16();
  send_ts = parser.read_uint64();
  payload = parser.read_string();

  return true;
}

string Datagram::serialize_to_string() const
{
  string binary;
  binary.reserve(HEADER_SIZE + payload.size());

  binary += put_number(frame_id);
  binary += put_number(static_cast<uint8_t>(frame_type));
  binary += put_number(frag_id);
  binary += put_number(frag_cnt);
  binary += put_number(send_ts);
  binary += payload;

  return binary;
}

size_t Msg::serialized_size() const
{
  return sizeof(type);
}

string Msg::serialize_to_string() const
{
  return put_number(static_cast<uint8_t>(type));
}

shared_ptr<Msg> Msg::parse_from_string(const string & binary)
{
  if (binary.size() < sizeof(type)) {
    return nullptr;
  }

  WireParser parser(binary);
  auto type = static_cast<Type>(parser.read_uint8());

  if (type == Type::ACK) {
    auto ret = make_shared<AckMsg>();
    ret->frame_id = parser.read_uint32();
    ret->frag_id = parser.read_uint16();
    ret->send_ts = parser.read_uint64();
    return ret;
  }
  else if (type == Type::CONFIG) {
    auto ret = make_shared<ConfigMsg>();
    ret->width = parser.read_uint16();
    ret->height = parser.read_uint16();
    ret->frame_rate = parser.read_uint16();
    ret->target_bitrate = parser.read_uint32();
    return ret;
  }
  else {
    return nullptr;
  }
}

AckMsg::AckMsg(const Datagram & datagram)
  : Msg(Type::ACK), frame_id(datagram.frame_id), frag_id(datagram.frag_id),
    send_ts(datagram.send_ts)
{}

size_t AckMsg::serialized_size() const
{
  return Msg::serialized_size() + sizeof(uint16_t) + sizeof(uint32_t)
         + sizeof(uint64_t);
}

string AckMsg::serialize_to_string() const
{
  string binary;
  binary.reserve(serialized_size());

  binary += Msg::serialize_to_string();
  binary += put_number(frame_id);
  binary += put_number(frag_id);
  binary += put_number(send_ts);

  return binary;
}

ConfigMsg::ConfigMsg(const uint16_t _width, const uint16_t _height,
                     const uint16_t _frame_rate, const uint32_t _target_bitrate)
  : Msg(Type::CONFIG), width(_width), height(_height),
    frame_rate(_frame_rate), target_bitrate(_target_bitrate)
{}

size_t ConfigMsg::serialized_size() const
{
  return Msg::serialized_size() + 3 * sizeof(uint16_t) + sizeof(uint32_t);
}

string ConfigMsg::serialize_to_string() const
{
  string binary;
  binary.reserve(serialized_size());

  binary += Msg::serialize_to_string();
  binary += put_number(width);
  binary += put_number(height);
  binary += put_number(frame_rate);
  binary += put_number(target_bitrate);

  return binary;
}
