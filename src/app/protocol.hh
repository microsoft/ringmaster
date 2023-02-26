#ifndef PROTOCOL_HH
#define PROTOCOL_HH

#include <string>
#include <memory>
#include <utility>

enum class FrameType : uint8_t {
  UNKNOWN = 0, // unknown
  KEY = 1,     // key frame
  NONKEY = 2,  // non-key frame
};

// uses (frame_id, frag_id) as sequence number
using SeqNum = std::pair<uint32_t, uint16_t>;

struct Datagram
{
  Datagram() {}
  Datagram(const uint32_t _frame_id,
           const FrameType _frame_type,
           const uint16_t _frag_id,
           const uint16_t _frag_cnt,
           const std::string_view _payload);

  uint32_t frame_id {};    // frame ID (1)
  FrameType frame_type {}; // frame type (2)
  uint16_t frag_id {};     // fragment ID in this frame (3)
  uint16_t frag_cnt {};    // total fragments in this frame (4)
  uint64_t send_ts {};     // timestamp (us) when the datagram is sent (5)
  std::string payload {};  // payload (6)

  // retransmission-related
  unsigned int num_rtx {0};
  uint64_t last_send_ts {0};

  // header size after serialization
  static constexpr size_t HEADER_SIZE = sizeof(uint32_t) +
      sizeof(FrameType) + 2 * sizeof(uint16_t) + sizeof(uint64_t);

  // maximum size for 'payload' (initialized in .cc and modified by set_mtu())
  static size_t max_payload;
  static void set_mtu(const size_t mtu);

  // construct this datagram by parsing binary string on wire
  bool parse_from_string(const std::string & binary);

  // serialize this datagram to binary string on wire
  std::string serialize_to_string() const;
};

struct Msg
{
  enum class Type : uint8_t {
    INVALID = 0, // invalid message type
    ACK = 1,     // AckMsg
    CONFIG = 2   // ConfigMsg
  };

  Type type {Type::INVALID}; // message type

  Msg() {}
  Msg(const Type _type) : type(_type) {}
  virtual ~Msg() {}

  // factory method to make a (derived class of) Msg
  static std::shared_ptr<Msg> parse_from_string(const std::string & binary);

  // virtual functions
  virtual size_t serialized_size() const;
  virtual std::string serialize_to_string() const;
};

struct AckMsg : Msg
{
  // construct an AckMsg
  AckMsg() : Msg(Type::ACK) {}
  AckMsg(const Datagram & datagram);

  uint32_t frame_id {}; // frame ID
  uint16_t frag_id {};  // fragment ID in this frame
  uint64_t send_ts {};  // timestamp (us) on sender when the datagram was sent

  size_t serialized_size() const override;
  std::string serialize_to_string() const override;
};

struct ConfigMsg : Msg
{
  // construct a ConfigMsg
  ConfigMsg() : Msg(Type::CONFIG) {}
  ConfigMsg(const uint16_t _width, const uint16_t _height,
            const uint16_t _frame_rate, const uint32_t _target_bitrate);

  uint16_t width {};          // display width
  uint16_t height {};         // display height
  uint16_t frame_rate {};     // FPS
  uint32_t target_bitrate {}; // target bitrate

  size_t serialized_size() const override;
  std::string serialize_to_string() const override;
};

#endif /* PROTOCOL_HH */
