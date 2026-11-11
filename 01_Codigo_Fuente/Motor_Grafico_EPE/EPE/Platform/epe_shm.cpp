#include "epe_shm.hpp"
#include <cstring>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace epe {
namespace plat {

std::string NormalizeMapName(const char* name_utf8) {
  if (!name_utf8 || !*name_utf8) return "/EPE_Canal_Tipo5_v1";
  std::string s(name_utf8);
  for (char& c : s) {
    if (c == '\\') c = '/';
  }
#if !defined(_WIN32)
  // POSIX shm exige nombre que empiece por '/'.
  if (s[0] != '/') s.insert(s.begin(), '/');
  // Evitar subcarpetas: aplanar.
  for (char& c : s) {
    if (c == '/' && &c != &s[0]) c = '_';
  }
#endif
  return s;
}

std::string WideToUtf8(const wchar_t* w) {
  if (!w) return {};
#if defined(_WIN32)
  int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
  if (n <= 1) return {};
  std::string out(static_cast<std::size_t>(n - 1), '\0');
  WideCharToMultiByte(CP_UTF8, 0, w, -1, out.data(), n, nullptr, nullptr);
  return out;
#else
  std::string out;
  for (const wchar_t* p = w; *p; ++p) {
    const unsigned u = static_cast<unsigned>(*p);
    if (u < 0x80) out.push_back(static_cast<char>(u));
    else if (u < 0x800) {
      out.push_back(static_cast<char>(0xC0 | (u >> 6)));
      out.push_back(static_cast<char>(0x80 | (u & 0x3F)));
    } else {
      out.push_back(static_cast<char>(0xE0 | (u >> 12)));
      out.push_back(static_cast<char>(0x80 | ((u >> 6) & 0x3F)));
      out.push_back(static_cast<char>(0x80 | (u & 0x3F)));
    }
  }
  return out;
#endif
}

std::wstring Utf8ToWide(const char* u8) {
  if (!u8) return {};
#if defined(_WIN32)
  int n = MultiByteToWideChar(CP_UTF8, 0, u8, -1, nullptr, 0);
  if (n <= 1) return {};
  std::wstring out(static_cast<std::size_t>(n - 1), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, u8, -1, out.data(), n);
  return out;
#else
  std::wstring out;
  for (const char* p = u8; *p; ++p) out.push_back(static_cast<wchar_t>(static_cast<unsigned char>(*p)));
  return out;
#endif
}

SharedMap::~SharedMap() { Close(); }

#if defined(_WIN32)

Status SharedMap::Open(const char* map_name_utf8, std::size_t bytes, bool create_if_missing) {
  Close();
  bytes_ = bytes;
  name_ = map_name_utf8 ? map_name_utf8 : "Local\\EPE_Canal_Tipo5_v1";
  const std::wstring w = Utf8ToWide(name_.c_str());
  HANDLE h = nullptr;
  if (create_if_missing) {
    h = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
                           static_cast<DWORD>(bytes), w.c_str());
  } else {
    h = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, w.c_str());
  }
  if (!h) return Status::FailClosed;
  void* view = MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, bytes);
  if (!view) {
    CloseHandle(h);
    return Status::FailClosed;
  }
  mapping_ = h;
  view_ = view;
  return Status::Ok;
}

void SharedMap::Close() {
  if (view_) {
    UnmapViewOfFile(view_);
    view_ = nullptr;
  }
  if (mapping_) {
    CloseHandle(static_cast<HANDLE>(mapping_));
    mapping_ = nullptr;
  }
  bytes_ = 0;
}

#else

Status SharedMap::Open(const char* map_name_utf8, std::size_t bytes, bool create_if_missing) {
  Close();
  bytes_ = bytes;
  name_ = NormalizeMapName(map_name_utf8);
  int flags = create_if_missing ? (O_CREAT | O_RDWR) : O_RDWR;
  fd_ = shm_open(name_.c_str(), flags, 0600);
  if (fd_ < 0) return Status::FailClosed;
  if (create_if_missing) {
    if (ftruncate(fd_, static_cast<off_t>(bytes)) != 0) {
      Close();
      return Status::FailClosed;
    }
  }
  void* view = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
  if (view == MAP_FAILED) {
    Close();
    return Status::FailClosed;
  }
  view_ = view;
  return Status::Ok;
}

void SharedMap::Close() {
  if (view_) {
    munmap(view_, bytes_);
    view_ = nullptr;
  }
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
  bytes_ = 0;
}

#endif

}  // namespace plat
}  // namespace epe
