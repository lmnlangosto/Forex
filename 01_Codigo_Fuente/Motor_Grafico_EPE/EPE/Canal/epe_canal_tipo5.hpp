#pragma once
// Contrato Doc_5 · canal tipo 5 (cabecera 48 B + ranuras A/B). Materializa Doc_5.
#include "epe_status.hpp"
#include "../../Public/epe_canal_layout.h"
#include <cstdint>

namespace epe {
namespace canal {

inline constexpr std::uint32_t kLayoutVersion = EPE_LAYOUT_VERSION;
inline constexpr std::uint32_t kHeaderBytes = EPE_HEADER_BYTES;
inline constexpr std::uint32_t kSlotPayloadCapacity = EPE_SLOT_PAYLOAD_CAPACITY;
inline constexpr std::uint32_t kSlotSize = EPE_SLOT_SIZE;
inline constexpr std::uint32_t kMapBytes = EPE_MAP_BYTES;
inline constexpr std::uint32_t kActiveSlotNone = EPE_ACTIVE_SLOT_NONE;
inline constexpr const wchar_t* kDefaultMapName = EPE_DEFAULT_MAP_NAME_W;

using Header = EpeCanalHeader;
using SlotPrefix = EpeSlotPrefix;

struct Snapshot {
  std::uint64_t generation = 0;
  std::uint64_t session_id = 0;
  std::uint32_t active_slot = kActiveSlotNone;
  std::uint32_t payload_bytes = 0;
  std::uint8_t* payload = nullptr;
};

enum class Role { Consumer, Producer };

class Channel {
 public:
  Channel() = default;
  ~Channel();
  Channel(const Channel&) = delete;
  Channel& operator=(const Channel&) = delete;

  Status Open(Role role, const wchar_t* map_name, std::uint64_t session_id,
              std::uint32_t layout_version = kLayoutVersion, bool create_if_missing = true);
  Status OpenUtf8(Role role, const char* map_name_utf8, std::uint64_t session_id,
                  std::uint32_t layout_version = kLayoutVersion, bool create_if_missing = true);
  void Close();

  bool IsOpen() const;
  Header* Hdr();
  const Header* Hdr() const;

  Status ConsumerTakeSnapshot(Snapshot& out, std::uint8_t* payload_buf, std::uint32_t payload_cap);
  Status ProducerPublishMinimal(std::uint64_t new_generation);
  Status ReinitSessionBump();

  std::uint64_t MatchedSession() const { return matched_session_; }
  std::uint32_t MatchedLayout() const { return matched_layout_; }

 private:
  struct Impl;
  Impl* impl_ = nullptr;
  Role role_ = Role::Consumer;
  std::uint64_t matched_session_ = 0;
  std::uint32_t matched_layout_ = 0;
  std::uint64_t last_accepted_generation_ = 0;

  std::uint8_t* SlotBase(std::uint32_t slot) const;
};

}  // namespace canal
}  // namespace epe
