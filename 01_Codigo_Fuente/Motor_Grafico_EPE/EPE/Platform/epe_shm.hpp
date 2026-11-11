#pragma once
#include "epe_status.hpp"
#include <cstddef>
#include <cstdint>
#include <string>

namespace epe {
namespace plat {

// Mapa compartido Doc_5: Win32 pagefile MMF o POSIX shm_open.
class SharedMap {
 public:
  SharedMap() = default;
  ~SharedMap();
  SharedMap(const SharedMap&) = delete;
  SharedMap& operator=(const SharedMap&) = delete;

  Status Open(const char* map_name_utf8, std::size_t bytes, bool create_if_missing);
  void Close();
  bool IsOpen() const { return view_ != nullptr; }
  void* View() { return view_; }
  const void* View() const { return view_; }
  std::size_t Bytes() const { return bytes_; }

 private:
  void* mapping_ = nullptr;
  void* view_ = nullptr;
  std::size_t bytes_ = 0;
  int fd_ = -1;
  std::string name_;
};

// Convierte nombre Windows "Local\\EPE_..." a nombre POSIX "/EPE_...".
std::string NormalizeMapName(const char* name_utf8);
std::string WideToUtf8(const wchar_t* w);
std::wstring Utf8ToWide(const char* u8);

}  // namespace plat
}  // namespace epe
