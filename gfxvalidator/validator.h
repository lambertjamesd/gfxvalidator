
#ifndef _GFX_VALIDATOR_VALIDATOR_H
#define _GFX_VALIDATOR_VALIDATOR_H

#include <ultra64.h>

#define GFX_MAX_COMMAND_LEN     256

#define GFX_MAX_SEGMENTS        16
#define GFX_MAX_GFX_STACK       10
#define GFX_MAX_MATRIX_STACK    10

#define GFX_INITIALIZED_PMTX    (1 << 0)
#define GFX_INITIALIZED_MMTX    (1 << 1)

#define GFX_MAX_REASON_LENGTH   96

enum GFXValidatorError {
    GFXValidatorErrorNone,
    GFXValidatorStackOverflow,
    GFXValidatorStackUnderflow,
    GFXValidatorInvalidCommand,
    GFXValidatorSegmentError,
    GFXValidatorDataAlignment,
    GFXValidatorInvalidAddress,
    GFXValidatorInvalidArguments,
    GFXValidatorUnitialized,
    GFXValidatorErrorCount,
};

struct GFXValidationResult {
    Gfx* gfxStack[GFX_MAX_GFX_STACK];
    char gfxStackSize;
    enum GFXValidatorError reason;
    char reasonMessage[GFX_MAX_REASON_LENGTH];
};

struct GFXValidatorState {
    struct GFXValidationResult* result;
    int segments[GFX_MAX_SEGMENTS];
    short matrixStackSize;
    int flags;
};

typedef void (*gfxPrinter)(char* output, unsigned outputLength);

enum GFXValidatorError gfxValidate(OSTask* task, int maxGfxCount, struct GFXValidationResult* result);
void gfxGenerateReadableMessage(struct GFXValidationResult* result, gfxPrinter printer);

#endif