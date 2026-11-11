#pragma once
#include <cstdint>

namespace epe {

enum class Status : std::int32_t {
  Ok = 0,
  FailClosed = 1,
  InvalidArg = 2,
  NotReady = 3,
  DeviceLost = 4,
  ChannelInsane = 5,
  LayoutMismatch = 6,
  SessionMismatch = 7,
  NoNewData = 8,
  OmittedPresent = 9,
  Unsupported = 10,
  InternalError = 11
};

inline const char* StatusName(Status s) {
  switch (s) {
    case Status::Ok: return "Ok";
    case Status::FailClosed: return "FailClosed";
    case Status::InvalidArg: return "InvalidArg";
    case Status::NotReady: return "NotReady";
    case Status::DeviceLost: return "DeviceLost";
    case Status::ChannelInsane: return "ChannelInsane";
    case Status::LayoutMismatch: return "LayoutMismatch";
    case Status::SessionMismatch: return "SessionMismatch";
    case Status::NoNewData: return "NoNewData";
    case Status::OmittedPresent: return "OmittedPresent";
    case Status::Unsupported: return "Unsupported";
    case Status::InternalError: return "InternalError";
  }
  return "Unknown";
}

}  // namespace epe
