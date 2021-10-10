
#ifndef _GFX_VALIDATOR_VALIDATOR_H
#define _GFX_VALIDATOR_VALIDATOR_H

#include <ultra64.h>

#define GFX_MAX_SEGMENTS        16
#define GFX_MAX_GFX_STACK       10
#define GFX_MAX_MATRIX_STACK    10

#define GFX_INITIALIZED_PMTX    (1 << 0)
#define GFX_INITIALIZED_MMTX    (1 << 1)

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
};

struct GFXValidationResult {
    Gfx* gfxStack[GFX_MAX_GFX_STACK];
    char gfxStackSize;
};

struct GFXValidatorState {
    struct GFXValidationResult* result;
    int segments[GFX_MAX_SEGMENTS];
    short matrixStackSize;
    int flags;
};

typedef enum GFXValidatorError (*CommandValidator)(struct GFXValidatorState* state, Gfx* at);

enum GFXValidatorError gfxValidate(OSTask* task, int maxGfxCount, struct GFXValidationResult* result);

#endif