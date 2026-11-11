#pragma once
#include <cstdint>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace epe {
namespace blind {

struct Config {
  std::uint32_t width = 1280;
  std::uint32_t height = 720;
  std::uint64_t channel_session_id = 1;
  std::uint32_t layout_version = 1;
  std::uint32_t create_channel = 0;
  std::uint32_t telemetry_capacity = 64;
  std::string map_name = "Local\\EPE_Canal_Tipo5_v1";
  std::string control_host = "127.0.0.1";
  int control_port = 17900;
  std::string orders_dir;
  std::string status_dir;
  bool headless = true;
  bool enable_control_socket = true;
  bool self_test = false;
  std::uint32_t auto_tick_ms = 0;
};

inline std::string Trim(std::string s) {
  while (!s.empty() && (s.back() == '\r' || s.back() == '\n' || s.back() == ' ' || s.back() == '\t'))
    s.pop_back();
  std::size_t i = 0;
  while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
  return s.substr(i);
}

inline bool LoadConfigFile(const std::string& path, Config& out) {
  std::ifstream in(path);
  if (!in) return false;
  std::string line;
  while (std::getline(in, line)) {
    line = Trim(line);
    if (line.empty() || line[0] == '#' || line[0] == ';') continue;
    const auto eq = line.find('=');
    if (eq == std::string::npos) continue;
    const std::string k = Trim(line.substr(0, eq));
    const std::string v = Trim(line.substr(eq + 1));
    if (k == "width") out.width = static_cast<std::uint32_t>(std::stoul(v));
    else if (k == "height") out.height = static_cast<std::uint32_t>(std::stoul(v));
    else if (k == "session_id" || k == "channel_session_id")
      out.channel_session_id = std::stoull(v);
    else if (k == "layout_version") out.layout_version = static_cast<std::uint32_t>(std::stoul(v));
    else if (k == "create_channel") out.create_channel = static_cast<std::uint32_t>(std::stoul(v));
    else if (k == "telemetry_capacity")
      out.telemetry_capacity = static_cast<std::uint32_t>(std::stoul(v));
    else if (k == "map_name") out.map_name = v;
    else if (k == "control_host") out.control_host = v;
    else if (k == "control_port") out.control_port = std::stoi(v);
    else if (k == "orders_dir") out.orders_dir = v;
    else if (k == "status_dir") out.status_dir = v;
    else if (k == "headless") out.headless = (v == "1" || v == "true" || v == "yes");
    else if (k == "enable_control_socket")
      out.enable_control_socket = (v == "1" || v == "true" || v == "yes");
    else if (k == "self_test") out.self_test = (v == "1" || v == "true" || v == "yes");
    else if (k == "auto_tick_ms") out.auto_tick_ms = static_cast<std::uint32_t>(std::stoul(v));
  }
  return true;
}

}  // namespace blind
}  // namespace epe
