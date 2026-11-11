#include "epe_canal_tipo5.hpp"
#include "../Platform/epe_atomics.hpp"
#include "../Platform/epe_shm.hpp"
#include <cstring>
#include <memory>

namespace epe {
namespace canal {

struct Channel::Impl {
  plat::SharedMap map;
};

Channel::~Channel() { Close(); }

bool Channel::IsOpen() const { return impl_ && impl_->map.IsOpen(); }

Header* Channel::Hdr() {
  return reinterpret_cast<Header*>(impl_ ? impl_->map.View() : nullptr);
}
const Header* Channel::Hdr() const {
  return reinterpret_cast<const Header*>(impl_ ? impl_->map.View() : nullptr);
}

std::uint8_t* Channel::SlotBase(std::uint32_t slot) const {
  auto* base = reinterpret_cast<std::uint8_t*>(impl_->map.View());
  return base + kHeaderBytes + static_cast<std::size_t>(slot) * kSlotSize;
}

Status Channel::Open(Role role, const wchar_t* map_name, std::uint64_t session_id,
                     std::uint32_t layout_version, bool create_if_missing) {
  const std::string utf8 = plat::WideToUtf8(map_name ? map_name : kDefaultMapName);
  return OpenUtf8(role, utf8.c_str(), session_id, layout_version, create_if_missing);
}

Status Channel::OpenUtf8(Role role, const char* map_name_utf8, std::uint64_t session_id,
                         std::uint32_t layout_version, bool create_if_missing) {
  Close();
  role_ = role;
  matched_session_ = session_id;
  matched_layout_ = layout_version;
  last_accepted_generation_ = 0;
  impl_ = new Impl();
  const Status st = impl_->map.Open(map_name_utf8, kMapBytes, create_if_missing);
  if (st != Status::Ok) {
    delete impl_;
    impl_ = nullptr;
    return st;
  }

  auto* hdr = Hdr();
  if (create_if_missing && role == Role::Producer) {
    std::memset(impl_->map.View(), 0, kMapBytes);
    hdr->publish_seq = 0;
    hdr->generation = 0;
    hdr->ack_seq = 0;
    hdr->consumed_generation = 0;
    hdr->layout_version = layout_version;
    hdr->active_slot = kActiveSlotNone;
    hdr->session_id = session_id;
  } else {
    plat::FullBarrier();
    if (hdr->layout_version != 0 && hdr->layout_version != layout_version) {
      Close();
      return Status::LayoutMismatch;
    }
    if (create_if_missing && hdr->session_id == 0 && hdr->layout_version == 0) {
      std::memset(impl_->map.View(), 0, kMapBytes);
      hdr->layout_version = layout_version;
      hdr->active_slot = kActiveSlotNone;
      hdr->session_id = session_id;
    } else if (hdr->layout_version != layout_version) {
      Close();
      return Status::LayoutMismatch;
    }
  }
  return Status::Ok;
}

void Channel::Close() {
  if (impl_) {
    impl_->map.Close();
    delete impl_;
    impl_ = nullptr;
  }
}

Status Channel::ConsumerTakeSnapshot(Snapshot& out, std::uint8_t* payload_buf,
                                     std::uint32_t payload_cap) {
  if (!IsOpen() || !payload_buf) return Status::InvalidArg;
  auto* hdr = Hdr();

  for (int attempt = 0; attempt < 64; ++attempt) {
    const std::uint64_t l1 = plat::AtomicLoadU64(&hdr->publish_seq);
    if (l1 & 1ull) continue;

    const std::uint64_t generation = hdr->generation;
    const std::uint32_t active = hdr->active_slot;
    const std::uint64_t session = hdr->session_id;
    const std::uint32_t layout = hdr->layout_version;

    if (layout != matched_layout_) return Status::LayoutMismatch;
    if (session != matched_session_) return Status::SessionMismatch;

    std::uint32_t copy_bytes = 0;
    if (active == kActiveSlotNone) {
      plat::FullBarrier();
      const std::uint64_t l2 = plat::AtomicLoadU64(&hdr->publish_seq);
      if (l1 != l2 || (l2 & 1ull)) continue;
      out.generation = generation;
      out.session_id = session;
      out.active_slot = active;
      out.payload_bytes = 0;
      out.payload = payload_buf;
      return Status::NoNewData;
    }
    if (active != 0 && active != 1) return Status::ChannelInsane;

    const std::uint8_t* slot = SlotBase(active);
    const std::uint8_t* payload = slot + 8;
    std::uint32_t total = 0;
    std::memcpy(&total, payload, sizeof(total));
    if (total > kSlotPayloadCapacity || total > payload_cap) return Status::ChannelInsane;
    if (total > 0) {
      std::memcpy(payload_buf, payload, total);
      copy_bytes = total;
    }

    plat::FullBarrier();
    const std::uint64_t l2 = plat::AtomicLoadU64(&hdr->publish_seq);
    if (l1 != l2 || (l2 & 1ull)) continue;

    const std::uint64_t odd = (hdr->ack_seq | 1ull);
    plat::AtomicStoreU64(&hdr->ack_seq, odd);
    hdr->consumed_generation = generation;
    plat::FullBarrier();
    plat::AtomicStoreU64(&hdr->ack_seq, odd + 1ull);

    out.generation = generation;
    out.session_id = session;
    out.active_slot = active;
    out.payload_bytes = copy_bytes;
    out.payload = payload_buf;

    if (generation <= last_accepted_generation_) return Status::NoNewData;
    last_accepted_generation_ = generation;
    return Status::Ok;
  }
  return Status::ChannelInsane;
}

Status Channel::ProducerPublishMinimal(std::uint64_t new_generation) {
  if (!IsOpen()) return Status::InvalidArg;
  auto* hdr = Hdr();
  if (hdr->layout_version != matched_layout_) return Status::LayoutMismatch;

  const std::uint32_t slot = (hdr->active_slot == 0) ? 1u : 0u;
  const std::uint64_t base = hdr->publish_seq & ~1ull;
  plat::AtomicStoreU64(&hdr->publish_seq, base + 1ull);

  std::uint8_t* slot_base = SlotBase(slot);
  auto* prefix = reinterpret_cast<SlotPrefix*>(slot_base);
  prefix->slot_generation = new_generation;

  std::uint8_t* payload = slot_base + 8;
  std::memset(payload, 0, EPE_PAYLOAD_ROOT_MIN_BYTES);
  std::uint32_t total = EPE_PAYLOAD_ROOT_MIN_BYTES;
  std::memcpy(payload, &total, sizeof(total));

  hdr->generation = new_generation;
  hdr->active_slot = slot;
  hdr->session_id = matched_session_;
  hdr->layout_version = matched_layout_;
  plat::FullBarrier();
  plat::AtomicStoreU64(&hdr->publish_seq, base + 2ull);
  return Status::Ok;
}

Status Channel::ReinitSessionBump() {
  if (!IsOpen()) return Status::NotReady;
  auto* hdr = Hdr();
  if (hdr->session_id == UINT64_MAX) return Status::FailClosed;
  const std::uint64_t next = hdr->session_id + 1ull;
  std::memset(impl_->map.View(), 0, kMapBytes);
  matched_session_ = next;
  hdr->session_id = next;
  hdr->layout_version = matched_layout_;
  hdr->active_slot = kActiveSlotNone;
  hdr->generation = 0;
  hdr->consumed_generation = 0;
  hdr->publish_seq = 0;
  hdr->ack_seq = 0;
  last_accepted_generation_ = 0;
  return Status::Ok;
}

}  // namespace canal
}  // namespace epe
