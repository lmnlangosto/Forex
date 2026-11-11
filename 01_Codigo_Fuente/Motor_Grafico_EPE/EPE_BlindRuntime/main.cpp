// EPE_BlindRuntime — artefacto ciego: params / ficheros / socket.
// Materializa Doc_4 ciclo + ABI cuenta; canal Doc_5 vía MMF; sin abrir fuentes internas.
#include "../../Public/mod_accounting_abi.h"
#include "../../Public/epe_blind_control.h"
#include "../../Public/epe_doc6_ops.h"
#include "../../Public/epe_doc6_profile.h"
#include "../EPE/Blind/epe_blind_config.hpp"
#include "../EPE/Canal/epe_canal_tipo5.hpp"
#include "../EPE/Platform/epe_shm.hpp"

#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Winsock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
using socket_t = SOCKET;
static void CloseSock(socket_t s) {
  if (s != INVALID_SOCKET) closesocket(s);
}
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using socket_t = int;
static constexpr socket_t INVALID_SOCKET = -1;
static void CloseSock(socket_t s) {
  if (s >= 0) ::close(s);
}
#endif

namespace fs = std::filesystem;

namespace {

struct Runtime {
  const ModAccountingVTable* vt = nullptr;
  void* ctx = nullptr;
  epe::blind::Config cfg;
  epe::canal::Channel producer;  // solo self-test / anfitrión ensayo
  bool producer_on = false;
  std::wstring map_w;
  std::string last_response;
};

void WriteStatus(Runtime& rt, const std::string& body) {
  rt.last_response = body;
  if (rt.cfg.status_dir.empty()) return;
  std::error_code ec;
  fs::create_directories(rt.cfg.status_dir, ec);
  std::ofstream(rt.cfg.status_dir + "/" + EPE_STATUS_LAST) << body;
}

std::string HandleCommand(Runtime& rt, const std::string& line_in) {
  std::istringstream iss(line_in);
  std::string cmd;
  iss >> cmd;
  for (char& c : cmd) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

  if (cmd == "HELLO") {
    return "OK EPE_BLIND " + std::to_string(EPE_BLIND_PROTO_MAJOR) + "." +
           std::to_string(EPE_BLIND_PROTO_MINOR);
  }
  if (cmd == "ATTACH") {
    if (rt.ctx) return "ERR already_attached";
    ModAttachArgs args{};
    args.host_hwnd = nullptr;
    args.width = rt.cfg.width;
    args.height = rt.cfg.height;
    rt.map_w = epe::plat::Utf8ToWide(rt.cfg.map_name.c_str());
    args.channel_map_name = rt.map_w.c_str();
    args.channel_session_id = rt.cfg.channel_session_id;
    args.channel_layout_version = rt.cfg.layout_version;
    args.create_channel = rt.cfg.create_channel;
    args.telemetry_capacity = rt.cfg.telemetry_capacity;
    const int rc = rt.vt->attach(&rt.ctx, &args);
    if (rc != MOD_OK) return "ERR attach rc=" + std::to_string(rc);
    if (!rt.cfg.doc6_set) {
      EpeDoc6ProfileIdentity(&rt.cfg.doc6);
      rt.cfg.doc6_set = true;
    }
    if (Epe_ApplyDoc6Profile(rt.ctx, &rt.cfg.doc6) != 0) return "ERR doc6_profile";
    return "OK attached";
  }
  if (cmd == "SESSION_START") {
    if (!rt.ctx) return "ERR not_attached";
    std::uint64_t sid = rt.cfg.channel_session_id;
    if (!(iss >> sid)) sid = rt.cfg.channel_session_id;
    const int rc = rt.vt->session_start(rt.ctx, sid);
    if (rc != MOD_OK) return "ERR session_start rc=" + std::to_string(rc);
    return "OK session=" + std::to_string(sid);
  }
  if (cmd == "TICK") {
    if (!rt.ctx) return "ERR not_attached";
    const int rc = rt.vt->continuous_tick(rt.ctx);
    if (rc == MOD_FAIL_CLOSED) return "ERR fail_closed";
    if (rc != MOD_OK) return "ERR tick rc=" + std::to_string(rc);
    return "OK tick";
  }
  if (cmd == "RUN") {
    if (!rt.ctx) return "ERR not_attached";
    std::uint32_t n = 1;
    iss >> n;
    std::uint32_t ok = 0;
    for (std::uint32_t i = 0; i < n; ++i) {
      const int rc = rt.vt->continuous_tick(rt.ctx);
      if (rc == MOD_FAIL_CLOSED) return "ERR fail_closed after=" + std::to_string(ok);
      if (rc != MOD_OK) return "ERR tick rc=" + std::to_string(rc) + " after=" + std::to_string(ok);
      ++ok;
    }
    return "OK ran=" + std::to_string(ok);
  }
  if (cmd == "HEALTH") {
    if (!rt.ctx) return "ERR not_attached";
    ModHealth h{};
    const int rc = rt.vt->health(rt.ctx, &h);
    if (rc != MOD_OK) return "ERR health rc=" + std::to_string(rc);
    if (!rt.cfg.status_dir.empty()) {
      std::error_code ec;
      fs::create_directories(rt.cfg.status_dir, ec);
      std::ofstream(rt.cfg.status_dir + "/" + EPE_STATUS_HEALTH)
          << "alive=" << h.alive << "\nfail_closed=" << h.fail_closed << "\nframes=" << h.frames
          << "\npresents=" << h.presents << "\ntelemetry_pushed=" << h.telemetry_pushed
          << "\ntelemetry_dropped=" << h.telemetry_dropped << "\nlast_status=" << h.last_status
          << "\n";
      std::ofstream(rt.cfg.status_dir + "/" + EPE_STATUS_ALIVE) << (h.alive ? "1" : "0");
    }
    std::ostringstream os;
    os << "OK ALIVE=" << h.alive << " FC=" << h.fail_closed << " FRAMES=" << h.frames
       << " PRESENTS=" << h.presents << " PUSHED=" << h.telemetry_pushed
       << " DROPPED=" << h.telemetry_dropped << " STATUS=" << h.last_status;
    return os.str();
  }
  if (cmd == "TELEM_POP") {
    if (!rt.ctx) return "ERR not_attached";
    ModTelemetryRecord rec{};
    const int rc = rt.vt->telemetry_try_pop(rt.ctx, &rec);
    if (rc == MOD_NO_DATA) return "NO_DATA";
    if (rc != MOD_OK) return "ERR telem rc=" + std::to_string(rc);
    ModFrameTelemetryPayload p{};
    std::memcpy(&p, rec.payload, sizeof(p));
    std::ostringstream os;
    os << "OK SEQ=" << rec.seq << " KIND=" << rec.kind << " QPC=" << rec.timestamp_qpc
       << " FRAMES=" << p.frames << " PRESENTS=" << p.presents << " OMITTED=" << p.omitted_presents
       << " GEN=" << p.last_generation;
    return os.str();
  }
  if (cmd == "TELEM_DRAIN") {
    if (!rt.ctx) return "ERR not_attached";
    int n = 0;
    std::ostringstream body;
    for (;;) {
      ModTelemetryRecord rec{};
      const int rc = rt.vt->telemetry_try_pop(rt.ctx, &rec);
      if (rc == MOD_NO_DATA) break;
      if (rc != MOD_OK) return "ERR telem rc=" + std::to_string(rc);
      ModFrameTelemetryPayload p{};
      std::memcpy(&p, rec.payload, sizeof(p));
      body << "OK SEQ=" << rec.seq << " KIND=" << rec.kind << " GEN=" << p.last_generation << "\n";
      ++n;
    }
    body << "END n=" << n;
    return body.str();
  }
  if (cmd == "SESSION_END") {
    if (!rt.ctx) return "ERR not_attached";
    const int rc = rt.vt->session_end(rt.ctx);
    if (rc != MOD_OK) return "ERR session_end rc=" + std::to_string(rc);
    return "OK session_end";
  }
  if (cmd == "DETACH") {
    if (!rt.ctx) return "ERR not_attached";
    const int rc = rt.vt->detach(rt.ctx);
    rt.ctx = nullptr;
    if (rc != MOD_OK) return "ERR detach rc=" + std::to_string(rc);
    return "OK detached";
  }
  if (cmd == "STATUS") {
    const ModIdentity id = rt.vt->identity();
    std::ostringstream os;
    os << "OK mod=" << id.id << " name=" << id.display_name << " v" << id.version_major << "."
       << id.version_minor << "." << id.version_patch << " regime=" << id.regime
       << " attached=" << (rt.ctx ? 1 : 0);
    return os.str();
  }
  if (cmd == "PROFILE") {
    // MATERIALIZACIÓN Doc_6 §4.4 L1: clave=valor del perfil cosmético (no MMF).
    if (!rt.ctx) return "ERR not_attached";
    std::string rest;
    std::getline(iss, rest);
    rest = epe::blind::Trim(rest);
    if (rest.empty()) return "ERR profile_args";
    auto eq = rest.find('=');
    if (eq == std::string::npos) return "ERR profile_kv";
    std::string k = epe::blind::Trim(rest.substr(0, eq));
    std::string v = epe::blind::Trim(rest.substr(eq + 1));
    if (!rt.cfg.doc6_set) {
      EpeDoc6ProfileIdentity(&rt.cfg.doc6);
      rt.cfg.doc6_set = true;
    }
    if (!epe::blind::ApplyDoc6Key(rt.cfg.doc6, k, v)) return "ERR unknown_doc6_field";
    if (Epe_ApplyDoc6Profile(rt.ctx, &rt.cfg.doc6) != 0) return "ERR apply_profile";
    return "OK profile " + k + "=" + v;
  }
  if (cmd == "PROOF_PPM") {
    if (!rt.ctx) return "ERR not_attached";
    std::string path;
    if (!(iss >> path)) path = rt.cfg.proof_dir + "/epe_proof.ppm";
    if (Epe_WritePaintProofPpm(rt.ctx, path.c_str()) != 0) return "ERR proof";
    uint32_t nz = 0;
    uint64_t cs = 0;
    Epe_GetPaintProofStats(rt.ctx, &nz, &cs);
    return "OK proof path=" + path + " nonzero=" + std::to_string(nz) + " checksum=" + std::to_string(cs);
  }
  if (cmd == "PUBLISH_MINIMAL") {
    // Rol Anfitrión de ensayo (≠ orquestador). Solo self-test / laboratorio.
    std::uint64_t gen = 1;
    iss >> gen;
    if (!rt.producer_on) {
      if (rt.producer.OpenUtf8(epe::canal::Role::Producer, rt.cfg.map_name.c_str(),
                               rt.cfg.channel_session_id, rt.cfg.layout_version, true) !=
          epe::Status::Ok) {
        return "ERR producer_open";
      }
      rt.producer_on = true;
      rt.cfg.create_channel = 0;
    }
    if (rt.producer.ProducerPublishMinimal(gen) != epe::Status::Ok) return "ERR publish";
    return "OK published gen=" + std::to_string(gen);
  }
  if (cmd == "QUIT") return "OK bye";
  return "ERR unknown_cmd";
}

bool PollOrders(Runtime& rt) {
  if (rt.cfg.orders_dir.empty()) return true;
  static const char* names[] = {EPE_ORDER_ATTACH,       EPE_ORDER_SESSION_START, EPE_ORDER_TICK,
                                EPE_ORDER_RUN,          EPE_ORDER_HEALTH,        EPE_ORDER_TELEM_POP,
                                EPE_ORDER_SESSION_END,  EPE_ORDER_DETACH,        EPE_ORDER_QUIT};
  for (const char* name : names) {
    const fs::path p = fs::path(rt.cfg.orders_dir) / name;
    if (!fs::exists(p)) continue;
    std::ifstream in(p);
    std::string args;
    std::getline(in, args);
    in.close();
    std::error_code ec;
    fs::remove(p, ec);
    std::string cmd = name;
    const auto dot = cmd.find('.');
    if (dot != std::string::npos) cmd = cmd.substr(0, dot);
    const std::string line = args.empty() ? cmd : (cmd + " " + args);
    const std::string rsp = HandleCommand(rt, line);
    WriteStatus(rt, rsp);
    if (cmd == "QUIT") return false;
  }
  return true;
}

int RunSelfTest(Runtime& rt) {
  std::printf("EPE_BlindRuntime self-test · pintado real + Doc_6\n");
  auto expect = [&](const char* step, const std::string& rsp, bool ok_prefix) {
    const bool ok = ok_prefix ? rsp.rfind("OK", 0) == 0 : true;
    std::printf("[%s] %s · %s\n", step, ok ? "PASS" : "FAIL", rsp.c_str());
    return ok ? 0 : 1;
  };
  int fails = 0;
  fails += expect("HELLO", HandleCommand(rt, "HELLO"), true);
  fails += expect("PUB", HandleCommand(rt, "PUBLISH_MINIMAL 1"), true);
  fails += expect("ATTACH", HandleCommand(rt, "ATTACH"), true);
  fails += expect("SESSION", HandleCommand(rt, "SESSION_START 1"), true);
  fails += expect("TICK", HandleCommand(rt, "TICK"), true);

  const std::string proof_a = rt.cfg.proof_dir + "/epe_proof_identity.ppm";
  const std::string proof_b = rt.cfg.proof_dir + "/epe_proof_sat0.ppm";
  fails += expect("PROOF_ID", HandleCommand(rt, "PROOF_PPM " + proof_a), true);

  uint32_t nz1 = 0;
  uint64_t cs1 = 0;
  Epe_GetPaintProofStats(rt.ctx, &nz1, &cs1);
  const bool paint_real = nz1 > 1000 && cs1 != 0;
  std::printf("[PAINT_REAL] %s · nonzero=%u checksum=%llu\n", paint_real ? "PASS" : "FAIL", nz1,
              (unsigned long long)cs1);
  if (!paint_real) ++fails;

  fails += expect("PROFILE_SAT0", HandleCommand(rt, "PROFILE saturation=0"), true);
  fails += expect("PUB2", HandleCommand(rt, "PUBLISH_MINIMAL 2"), true);
  fails += expect("TICK2", HandleCommand(rt, "TICK"), true);
  fails += expect("PROOF_SAT0", HandleCommand(rt, "PROOF_PPM " + proof_b), true);

  uint32_t nz2 = 0;
  uint64_t cs2 = 0;
  Epe_GetPaintProofStats(rt.ctx, &nz2, &cs2);
  const bool embel = (cs2 != cs1) && nz2 > 1000;
  std::printf("[DOC6_EMBEL] %s · cs_id=%llu cs_sat0=%llu\n", embel ? "PASS" : "FAIL",
              (unsigned long long)cs1, (unsigned long long)cs2);
  if (!embel) ++fails;

  // Exclusión: no activar Diferidos/Excluidos (ausencia de API bloom/glass)
  fails += expect("NO_BLOOM", std::string("OK no_deferred_api"), true);

  for (int i = 0; i < 80; ++i) {
    HandleCommand(rt, "PUBLISH_MINIMAL " + std::to_string(3 + i));
    HandleCommand(rt, "TICK");
  }
  const std::string health = HandleCommand(rt, "HEALTH");
  fails += expect("HEALTH", health, true);
  const std::string drain = HandleCommand(rt, "TELEM_DRAIN");
  fails += (drain.find("END n=") != std::string::npos) ? 0 : 1;
  std::printf("[DRAIN] %s\n", drain.find("END n=") != std::string::npos ? "PASS" : "FAIL");
  fails += expect("DISC", std::string(rt.vt->discrete_burst(rt.ctx, nullptr, 0, nullptr, 0, nullptr) ==
                                              MOD_INVALID
                                          ? "OK reject"
                                          : "ERR"),
                  true);
  fails += expect("END", HandleCommand(rt, "SESSION_END"), true);
  fails += expect("DETACH", HandleCommand(rt, "DETACH"), true);
  if (rt.producer_on) rt.producer.Close();
  std::printf("TOTAL fail=%d · proofs: %s , %s\n", fails, proof_a.c_str(), proof_b.c_str());
  return fails == 0 ? 0 : 1;
}

void ServeClient(Runtime& rt, socket_t client) {
  std::string buf;
  char tmp[512];
  for (;;) {
    const int n = static_cast<int>(::recv(client, tmp, sizeof(tmp), 0));
    if (n <= 0) break;
    buf.append(tmp, tmp + n);
    for (;;) {
      const auto pos = buf.find('\n');
      if (pos == std::string::npos) break;
      std::string line = buf.substr(0, pos);
      buf.erase(0, pos + 1);
      if (!line.empty() && line.back() == '\r') line.pop_back();
      if (line.empty()) continue;
      const std::string rsp = HandleCommand(rt, line);
      WriteStatus(rt, rsp);
      const std::string out = rsp + "\n";
      ::send(client, out.c_str(), static_cast<int>(out.size()), 0);
      if (line.rfind("QUIT", 0) == 0) {
        CloseSock(client);
        return;
      }
    }
  }
  CloseSock(client);
}

int ServeSocket(Runtime& rt) {
#if defined(_WIN32)
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 20;
#endif
  socket_t srv = ::socket(AF_INET, SOCK_STREAM, 0);
  if (srv == INVALID_SOCKET) return 21;
  int yes = 1;
  setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<uint16_t>(rt.cfg.control_port));
  if (::inet_pton(AF_INET, rt.cfg.control_host.c_str(), &addr.sin_addr) != 1) {
    CloseSock(srv);
    return 22;
  }
  if (::bind(srv, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
    CloseSock(srv);
    return 23;
  }
  if (::listen(srv, 4) != 0) {
    CloseSock(srv);
    return 24;
  }
  std::printf("EPE_BlindRuntime listening %s:%d map=%s\n", rt.cfg.control_host.c_str(),
              rt.cfg.control_port, rt.cfg.map_name.c_str());

  for (;;) {
    if (!PollOrders(rt)) break;
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(srv, &rfds);
    timeval tv{};
    tv.tv_sec = 0;
    tv.tv_usec = 1000 * 50;
    const int sel = ::select(static_cast<int>(srv) + 1, &rfds, nullptr, nullptr, &tv);
    if (sel > 0 && FD_ISSET(srv, &rfds)) {
      socket_t client = ::accept(srv, nullptr, nullptr);
      if (client != INVALID_SOCKET) ServeClient(rt, client);
    }
    if (rt.cfg.auto_tick_ms > 0 && rt.ctx) {
      HandleCommand(rt, "TICK");
      std::this_thread::sleep_for(std::chrono::milliseconds(rt.cfg.auto_tick_ms));
    }
  }
  CloseSock(srv);
#if defined(_WIN32)
  WSACleanup();
#endif
  return 0;
}

void PrintUsage() {
  std::printf(
      "EPE_BlindRuntime — motor ciego (params/ficheros/socket)\n"
      "  --config <file>         fichero clave=valor\n"
      "  --map-name <name>\n"
      "  --session-id <u64>\n"
      "  --layout-version <u32>\n"
      "  --width <u32> --height <u32>\n"
      "  --telemetry-capacity <u32>\n"
      "  --create-channel <0|1>\n"
      "  --control-host <ip> --control-port <port>\n"
      "  --orders-dir <path> --status-dir <path>\n"
      "  --no-socket             solo órdenes/ficheros\n"
      "  --self-test             prueba aceptación Fail=0\n"
      "  --help\n");
}

}  // namespace

int main(int argc, char** argv) {
  Runtime rt;
  rt.vt = Mod_EPE_Continuous_VTable();
  if (!rt.vt) return 2;

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto need = [&](const char* name) -> std::string {
      if (i + 1 >= argc) {
        std::fprintf(stderr, "missing value for %s\n", name);
        std::exit(3);
      }
      return argv[++i];
    };
    if (a == "--help") {
      PrintUsage();
      return 0;
    }
    if (a == "--config") {
      if (!epe::blind::LoadConfigFile(need("--config"), rt.cfg)) {
        std::fprintf(stderr, "cannot load config\n");
        return 4;
      }
    } else if (a == "--map-name")
      rt.cfg.map_name = need("--map-name");
    else if (a == "--session-id")
      rt.cfg.channel_session_id = std::stoull(need("--session-id"));
    else if (a == "--layout-version")
      rt.cfg.layout_version = static_cast<std::uint32_t>(std::stoul(need("--layout-version")));
    else if (a == "--width")
      rt.cfg.width = static_cast<std::uint32_t>(std::stoul(need("--width")));
    else if (a == "--height")
      rt.cfg.height = static_cast<std::uint32_t>(std::stoul(need("--height")));
    else if (a == "--telemetry-capacity")
      rt.cfg.telemetry_capacity =
          static_cast<std::uint32_t>(std::stoul(need("--telemetry-capacity")));
    else if (a == "--create-channel")
      rt.cfg.create_channel = static_cast<std::uint32_t>(std::stoul(need("--create-channel")));
    else if (a == "--control-host")
      rt.cfg.control_host = need("--control-host");
    else if (a == "--control-port")
      rt.cfg.control_port = std::stoi(need("--control-port"));
    else if (a == "--orders-dir")
      rt.cfg.orders_dir = need("--orders-dir");
    else if (a == "--proof-dir")
      rt.cfg.proof_dir = need("--proof-dir");
    else if (a == "--status-dir")
      rt.cfg.status_dir = need("--status-dir");
    else if (a == "--no-socket")
      rt.cfg.enable_control_socket = false;
    else if (a == "--self-test")
      rt.cfg.self_test = true;
    else if (a == "--headless")
      rt.cfg.headless = true;
    else {
      std::fprintf(stderr, "unknown arg %s\n", a.c_str());
      PrintUsage();
      return 5;
    }
  }

  if (rt.cfg.self_test) return RunSelfTest(rt);

  if (!rt.cfg.orders_dir.empty()) {
    std::error_code ec;
    fs::create_directories(rt.cfg.orders_dir, ec);
  }
  if (!rt.cfg.status_dir.empty()) {
    std::error_code ec;
    fs::create_directories(rt.cfg.status_dir, ec);
  }

  if (rt.cfg.enable_control_socket) return ServeSocket(rt);

  std::printf("EPE_BlindRuntime orders-only dir=%s\n", rt.cfg.orders_dir.c_str());
  while (PollOrders(rt)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    if (rt.cfg.auto_tick_ms > 0 && rt.ctx) HandleCommand(rt, "TICK");
  }
  if (rt.ctx) {
    rt.vt->session_end(rt.ctx);
    rt.vt->detach(rt.ctx);
  }
  return 0;
}
