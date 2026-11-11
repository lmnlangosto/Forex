#pragma once
/* Contrato de control del artefacto ciego EPE.
 * Un tercero opera el motor SOLO con: binario + Public/*.h + pliegos de uso/DCD.
 * Materializa Doc_4 (Attach/Session/Runtime/Shutdown) y Doc_5 (canal aparte del control).
 */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Versión del protocolo de control (línea UTF-8). */
enum { EPE_BLIND_PROTO_MAJOR = 1, EPE_BLIND_PROTO_MINOR = 0 };

/*
 * Transporte:
 *  1) CLI / fichero de configuración (params).
 *  2) Directorio de órdenes (ficheros).
 *  3) Socket TCP de control (IPC) — una línea comando → una o más líneas respuesta.
 *
 * Canal Doc_5 (MMF) NO viaja por este socket: el Anfitrión publica en el MMF;
 * el EPE lo consume. El orquestador observa health/buzón por este control.
 */

/* Comandos de línea (mayúsculas, tokens separados por espacio). */
/*
 * HELLO
 * ATTACH
 * SESSION_START <session_id_u64>
 * TICK
 * RUN <n_ticks_u32>          — bombea n ticks (ensayo; producto Continuo puede hilo propio)
 * HEALTH
 * TELEM_POP                  — un registro; NO_DATA si vacío
 * TELEM_DRAIN                — vacía el anillo; responde N líneas OK + END n=
 * SESSION_END
 * DETACH
 * STATUS                     — resumen texto
 * QUIT
 */

/* Códigos de respuesta (prefijo de línea). */
enum {
  EPE_BLIND_RSP_OK = 0,
  EPE_BLIND_RSP_ERR = 1,
  EPE_BLIND_RSP_NO_DATA = 2
};

/* Nombres de fichero de órdenes (si --orders-dir). Contenido = argumentos en una línea. */
#define EPE_ORDER_ATTACH "ATTACH.order"
#define EPE_ORDER_SESSION_START "SESSION_START.order"
#define EPE_ORDER_TICK "TICK.order"
#define EPE_ORDER_RUN "RUN.order"
#define EPE_ORDER_HEALTH "HEALTH.order"
#define EPE_ORDER_TELEM_POP "TELEM_POP.order"
#define EPE_ORDER_SESSION_END "SESSION_END.order"
#define EPE_ORDER_DETACH "DETACH.order"
#define EPE_ORDER_QUIT "QUIT.order"

/* Ficheros de estado (si --status-dir). */
#define EPE_STATUS_HEALTH "health.txt"
#define EPE_STATUS_LAST "last_response.txt"
#define EPE_STATUS_ALIVE "alive.flag"

#ifdef __cplusplus
}
#endif
