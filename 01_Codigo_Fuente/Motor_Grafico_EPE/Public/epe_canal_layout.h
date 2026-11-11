#pragma once
/* Layout binario Doc_5 · cabecera 48 B + ranuras A/B.
 * Entrega pública: el Anfitrión puede publicar sin fuentes internas del EPE.
 * Norma: Doc_5 Diccionario del Canal (inviolable; este header la materializa).
 */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
  EPE_LAYOUT_VERSION = 1,
  EPE_HEADER_BYTES = 48,
  EPE_SLOT_PAYLOAD_CAPACITY = 3145728, /* 3 MiB */
  EPE_SLOT_SIZE = 3145736,             /* 8 + capacidad (SlotPrefix + payload) */
  EPE_MAP_BYTES = 6291520,             /* 48 + 2 * SLOT_SIZE */
  EPE_ACTIVE_SLOT_NONE = 0xFFFFFFFFu,
  EPE_PAYLOAD_ROOT_MIN_BYTES = 72
};

#pragma pack(push, 1)
typedef struct EpeCanalHeader {
  uint64_t publish_seq;          /* seqlock: impar=escritura; par=estable */
  uint64_t generation;           /* monótona P_N */
  uint64_t ack_seq;              /* seqlock ACK */
  uint64_t consumed_generation;  /* EPE escribe tras copia privada */
  uint32_t layout_version;
  uint32_t active_slot;          /* 0|1|0xFFFFFFFF */
  uint64_t session_id;
} EpeCanalHeader;
#pragma pack(pop)

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(EpeCanalHeader) == 48, "Doc_5 header must be 48 bytes");
#elif defined(__cplusplus)
static_assert(sizeof(EpeCanalHeader) == 48, "Doc_5 header must be 48 bytes");
#endif

#pragma pack(push, 1)
typedef struct EpeSlotPrefix {
  uint64_t slot_generation;
} EpeSlotPrefix;
#pragma pack(pop)

/* Nombre por defecto Windows; en POSIX el runtime mapea a /EPE_Canal_Tipo5_v1 */
#define EPE_DEFAULT_MAP_NAME_W L"Local\\EPE_Canal_Tipo5_v1"
#define EPE_DEFAULT_MAP_NAME_A "Local\\EPE_Canal_Tipo5_v1"

#ifdef __cplusplus
}
#endif
