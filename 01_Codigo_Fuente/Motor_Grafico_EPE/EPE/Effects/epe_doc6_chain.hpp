#pragma once
// Cadena cromática Doc_6 §5.1–§5.3 · MATERIALIZACIÓN Doc_6 (L4).
#include "../../Public/epe_doc6_profile.h"
#include <cstdint>

namespace epe {
namespace doc6 {

struct Rgba {
  float r, g, b, a;
};

inline float Saturate(float x) { return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); }

// Orden normativo: A08/A09 → A07 → A06 → A04 → A05 → A03 → A10 (+ A11 en cuantización).
Rgba ApplyChromaticChain(Rgba in, const EpeDoc6Profile& p);

// Aplica cadena a buffer RGBA8 premult. (A11 dither opcional).
void ApplyProfileToRgba8(std::uint8_t* rgba, std::uint32_t width, std::uint32_t height,
                         const EpeDoc6Profile& p);

}  // namespace doc6
}  // namespace epe
