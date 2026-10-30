#ifndef CFE_SB_H
#define CFE_SB_H

#include "../../include/common_types.h"
#include "../../include/osapi.h"
#include "../../include/exploidus_capability.h"
#include "cfe_error.h"

#define CFE_SB_MAX_PIPES         16
#define CFE_SB_MAX_MSG_SIZE      1024
#define CFE_SB_MAX_SUBSCRIPTIONS 64
#define CFE_SB_PIPE_DEPTH_MAX    32

#define CFE_SB_POLL              OS_CHECK
#define CFE_SB_PEND_FOREVER      OS_PEND

typedef uint32 CFE_SB_MsgId_t;
typedef uint8  CFE_SB_PipeId_t;

#define CFE_SB_INVALID_PIPE      0xFF

#pragma pack(push, 1)
typedef struct
{
    CFE_SB_MsgId_t MsgId;
    uint16         Size;
    uint16         Sequence;
    uint32         SenderApp;
    uint8          Data[CFE_SB_MAX_MSG_SIZE];
} CFE_SB_Buffer_t;
#pragma pack(pop)

/* Capability flags for Exploidus-gated bus endpoints */
#define CFE_SB_CAP_RESTRICTED_MASK  0x80000000U

/* Function Prototypes */
int32 CFE_SB_EarlyInit(void);
int32 CFE_SB_CreatePipe(CFE_SB_PipeId_t *PipeIdPtr, uint16 Depth, const char *PipeName);
int32 CFE_SB_DeletePipe(CFE_SB_PipeId_t PipeId);
int32 CFE_SB_Subscribe(CFE_SB_MsgId_t MsgId, CFE_SB_PipeId_t PipeId);
int32 CFE_SB_SubscribeGated(CFE_SB_MsgId_t MsgId, CFE_SB_PipeId_t PipeId, const exploidus_cap_token_t *CapToken);
int32 CFE_SB_Unsubscribe(CFE_SB_MsgId_t MsgId, CFE_SB_PipeId_t PipeId);
int32 CFE_SB_TransmitMsg(CFE_SB_Buffer_t *BufPtr, bool IncrementSequence);
int32 CFE_SB_TransmitMsgGated(CFE_SB_Buffer_t *BufPtr, bool IncrementSequence, const exploidus_cap_token_t *CapToken);
int32 CFE_SB_ReceiveBuffer(CFE_SB_Buffer_t **BufPtr, CFE_SB_PipeId_t PipeId, int32 TimeOut);
int32 CFE_SB_InitMsg(CFE_SB_Buffer_t *BufPtr, CFE_SB_MsgId_t MsgId, size_t Size, bool Clear);

#endif /* CFE_SB_H */
