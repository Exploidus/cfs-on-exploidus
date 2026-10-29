#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef uint8_t   uint8;
typedef int8_t    int8;
typedef uint16_t  uint16;
typedef int16_t   int16;
typedef uint32_t  uint32;
typedef int32_t   int32;
typedef uint64_t  uint64;
typedef int64_t   int64;

typedef uint32_t  osal_id_t;
typedef uint32_t  osal_index_t;

#define OS_OBJECT_ID_UNDEFINED 0

/* Exploidus capability security bitmasks */
#define EXPLOIDUS_CAP_READ      (1U << 0)
#define EXPLOIDUS_CAP_WRITE     (1U << 1)
#define EXPLOIDUS_CAP_EXECUTE   (1U << 2)
#define EXPLOIDUS_CAP_EFFECTOR  (1U << 3) /* Flight critical effector authorization */
#define EXPLOIDUS_CAP_TELEMETRY (1U << 4)

#endif /* COMMON_TYPES_H */
