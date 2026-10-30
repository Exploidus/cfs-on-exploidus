#ifndef CFE_ERROR_H
#define CFE_ERROR_H

#include "../../include/common_types.h"
#include "../../include/osapi-error.h"

#define CFE_SUCCESS                   OS_SUCCESS

/* Software Bus (SB) Errors */
#define CFE_SB_ERROR                  (-0x2001)
#define CFE_SB_BAD_ARGUMENT           (-0x2002)
#define CFE_SB_MAX_PIPES_MET          (-0x2003)
#define CFE_SB_PIPE_RD_ERR            (-0x2004)
#define CFE_SB_NO_MESSAGE             (-0x2005)
#define CFE_SB_MSG_TOO_BIG            (-0x2006)
#define CFE_SB_BUF_ALIGNED_ERR        (-0x2007)
#define CFE_SB_INTERNAL_ERR           (-0x2008)
#define CFE_SB_ACCESS_DENIED          (-0x2009)

/* Executive Services (ES) Errors */
#define CFE_ES_ERROR                  (-0x3001)
#define CFE_ES_BAD_ARGUMENT           (-0x3002)
#define CFE_ES_MAX_APPLICATIONS_MET   (-0x3003)
#define CFE_ES_RESOURCE_ERROR         (-0x3004)
#define CFE_ES_ERR_APP_NOT_FOUND      (-0x3005)

/* Event Services (EVS) Errors */
#define CFE_EVS_ERROR                 (-0x4001)
#define CFE_EVS_UNKNOWN_FLAG          (-0x4002)
#define CFE_EVS_APP_NOT_REGISTERED    (-0x4003)
#define CFE_EVS_INVALID_PARAMETER     (-0x4004)

#endif /* CFE_ERROR_H */
