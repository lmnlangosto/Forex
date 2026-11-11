#pragma once
/* Perfil cosmético Doc_6 §4.4 — MATERIALIZACIÓN Doc_6.
 * NO es payload MMF (R-6-02). Configuración L1/L2 del motor.
 * Aprobado v1: A01–A22. Excluidos/Diferidos: ausentes.
 */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct EpeDoc6Profile {
  float color_gain_rgb[3]; /* A06 · default 1,1,1 · ≥0 */
  float saturation;        /* A05 · default 1 · ≥0 */
  float contrast;          /* A04 · default 1 · >0 */
  float gamma;             /* A03 · default 1 · >0 */
  float offset[3];         /* A07 · default 0,0,0 */
  float black_point;       /* A08 · [0,1] default 0 */
  float white_point;       /* A09 · (black,1] default 1 */
  float exposure;          /* A10 · default 0 stops */
  float opacity_default;   /* A13 · [0,1] default 1 */
  uint32_t dither_enable;  /* A11 · 0 off / 1 on */
  uint32_t aa_primitives;  /* A22 · 1 on (Init) */
  uint32_t lut_id;         /* A01 · 0 = identidad */
} EpeDoc6Profile;

static inline void EpeDoc6ProfileIdentity(EpeDoc6Profile* p) {
  p->color_gain_rgb[0] = p->color_gain_rgb[1] = p->color_gain_rgb[2] = 1.f;
  p->saturation = 1.f;
  p->contrast = 1.f;
  p->gamma = 1.f;
  p->offset[0] = p->offset[1] = p->offset[2] = 0.f;
  p->black_point = 0.f;
  p->white_point = 1.f;
  p->exposure = 0.f;
  p->opacity_default = 1.f;
  p->dither_enable = 0;
  p->aa_primitives = 1;
  p->lut_id = 0;
}

#ifdef __cplusplus
}
#endif
