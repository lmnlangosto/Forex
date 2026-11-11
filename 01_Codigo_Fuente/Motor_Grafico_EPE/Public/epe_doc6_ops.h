#pragma once
/* Operaciones Doc_6 L1/L2 · MATERIALIZACIÓN del perfil cosmético §4.4.
 * No es API de orquestador inventada: aplica el perfil Doc_6 al motor ya attached.
 * R-6-02: estos campos NO van al MMF Doc_5.
 */
#include "epe_doc6_profile.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ctx = contexto devuelto por ModAccountingVTable.attach */
int Epe_ApplyDoc6Profile(void* ctx, const EpeDoc6Profile* profile); /* 0=ok */
int Epe_WritePaintProofPpm(void* ctx, const char* path);           /* 0=ok */
int Epe_GetPaintProofStats(void* ctx, uint32_t* nonzero_pixels, uint64_t* checksum);

#ifdef __cplusplus
}
#endif
