#ifndef OSAPI_ERROR_H
#define OSAPI_ERROR_H

#include "common_types.h"

/* Standard NASA cFS OSAL return codes */
#define OS_SUCCESS                     0
#define OS_ERROR                      (-1)
#define OS_INVALID_POINTER            (-2)
#define OS_ERR_NAME_TOO_LONG          (-3)
#define OS_ERR_NO_FREE_IDS            (-4)
#define OS_ERR_NAME_TAKEN             (-5)
#define OS_ERR_INVALID_ID             (-6)
#define OS_ERR_NAME_NOT_FOUND         (-7)
#define OS_ERR_SEM_TAKEN              (-8)
#define OS_ERR_SEM_EMPTY              (-9)
#define OS_ERR_TIMEOUT                (-10)
#define OS_QUEUE_EMPTY                (-11)
#define OS_QUEUE_FULL                 (-12)
#define OS_QUEUE_TIMEOUT              (-13)
#define OS_QUEUE_INVALID_SIZE         (-14)

/* Exploidus Capability Security Error Codes */
#define OS_ERR_CAPABILITY_DENIED      (-100)
#define OS_ERR_CAPABILITY_EXPIRED     (-101)
#define OS_ERR_EFFECTOR_UNAUTHORIZED  (-102)

#endif /* OSAPI_ERROR_H */
