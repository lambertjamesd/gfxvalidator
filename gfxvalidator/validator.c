
#include "validator.h"
#include <string.h>
#include "gfx_macros.h"

typedef enum GFXValidatorError (*CommandValidator)(struct GFXValidatorState* state, Gfx* at);

extern CommandValidator gfxCommandValidators[GFX_MAX_COMMAND_LEN];


void gfxInitState(struct GFXValidatorState* state, struct GFXValidationResult* result) {
    int i;

    for (i = 0; i < GFX_MAX_SEGMENTS; ++i) {
        state->segments[i] = SEGMENT_UNINITIALIZED;
    }

    state->result = result;

    state->matrixStackSize = 0;
    state->result->gfxStackSize = 0;
    state->result->reason = GFXValidatorErrorNone;
    state->result->reasonMessage[0] = 0;
    state->flags = 0;

}

enum GFXValidatorError gfxPush(struct GFXValidatorState* state, Gfx* next) {
    if (state->result->gfxStackSize == GFX_MAX_GFX_STACK) {
        sprintf(state->result->reasonMessage, "display list stack overflow");
        return GFXValidatorStackOverflow;
    } else {
        state->result->gfxStack[(int)state->result->gfxStackSize++] = next;
        return GFXValidatorErrorNone;
    }
}

void gfxPop(struct GFXValidatorState* state) {
    state->result->gfxStackSize--;
}

int gfxIsAligned(int addr, int to) {
    return (addr & ~(to - 1)) == addr;
}

int gfxIsInRam(int addr) {
    addr = addr & 0xFFFFFFF;
    return addr > 0 && addr < osMemSize;
}

int gfxIsValidSegmentAddress(int addr) {
    return gfxIsInRam(addr) || addr == 0;
}

enum GFXValidatorError gfxTranslateAddress(struct GFXValidatorState* state, int address, int* output) {
    int segment = _SHIFTR(address, 24, 4);

    if (segment < 0 || segment >= 16 || state->segments[segment] == SEGMENT_UNINITIALIZED) {
        sprintf(state->result->reasonMessage, "attempt to use segment 0x%x before it was initialized", segment);
        return GFXValidatorSegmentError;
    } else {
        *output = state->segments[segment] + (address & 0xFFFFFF);
    }

    return GFXValidatorErrorNone;
}

enum GFXValidatorError gfxValidateAddress(struct GFXValidatorState* state, int address, int alignedTo) {
    int translated;
    enum GFXValidatorError result = gfxTranslateAddress(state, address, &translated);

    if (result != GFXValidatorErrorNone) {
        return result;
    }

    if (!gfxIsAligned(translated, alignedTo)) {
        sprintf(state->result->reasonMessage, "address 0x%08x must to aligned to %d bytes", address, alignedTo);
        return GFXValidatorDataAlignment;
    }

    if (!gfxIsInRam(translated)) {
        sprintf(state->result->reasonMessage, "address 0x%08x translates to 0x%08x which isn't in RAM", address, translated);
        return GFXValidatorInvalidAddress;
    }

    return GFXValidatorErrorNone;
}

enum GFXValidatorError gfxValidateNoop(struct GFXValidatorState* state, Gfx* at) {
    if (at->dma.addr != 0 || at->dma.len != 0 || at->dma.par != 0) {
        sprintf(state->result->reasonMessage, "nop instruction should equal 0");
        return GFXValidatorInvalidArguments;
    } else {
        return GFXValidatorErrorNone;
    }
}

enum GFXValidatorError gfxValidateMtx(struct GFXValidatorState* state, Gfx* at) {
    int flags = DMA_MM_IDX(at);

#ifdef F3DEX_GBI_2
    flags ^= G_MTX_PUSH;
#endif

    if (DMA_MM_LEN(at) != DMA_MM_EXPECTED_SIZE(sizeof(Mtx))) {
        sprintf(state->result->reasonMessage, "malformed matrix operation");
        return GFXValidatorInvalidArguments;
    } else if (flags < 0 || flags > (G_MTX_PROJECTION | G_MTX_LOAD | G_MTX_PUSH)) {
        sprintf(state->result->reasonMessage, "invalid matrix flags");
        return GFXValidatorInvalidArguments;
    } else if ((flags & G_MTX_PUSH) && state->matrixStackSize == GFX_MAX_MATRIX_STACK) {
        sprintf(state->result->reasonMessage, "matrix stack overflow");
        return GFXValidatorStackOverflow;
    } else if ((flags & G_MTX_PUSH) && (flags & G_MTX_PROJECTION)) {
        sprintf(state->result->reasonMessage, "cannot push a G_MTX_PROJECTION matrix");
        return GFXValidatorInvalidArguments;
    } else {
        if (!(flags & G_MTX_LOAD)) {
            if (flags & G_MTX_PROJECTION) {
                if (!(state->flags & GFX_INITIALIZED_PMTX)) {
                    sprintf(state->result->reasonMessage, "cannot multiply, no existing matrix exists");
                    return GFXValidatorUnitialized;
                }
            } else {
                if (!(state->flags & GFX_INITIALIZED_MMTX)) {
                    sprintf(state->result->reasonMessage, "cannot multiply, no existing matrix exists");
                    return GFXValidatorUnitialized;
                }
            }
        }

        if ((flags & G_MTX_PUSH)) {
            ++state->matrixStackSize;
        }

        if (flags & G_MTX_PROJECTION) {
            state->flags |= GFX_INITIALIZED_PMTX;
        } else {
            state->flags |= GFX_INITIALIZED_MMTX;
        }

        return gfxValidateAddress(state, at->dma.addr, 8);
    }
}

enum GFXValidatorError gfxValidateMoveMem(struct GFXValidatorState* state, Gfx* at) {
    int location = DMA_MM_IDX(at);
    int expectedLen = 0;

    switch (location) {
        case G_MV_VIEWPORT:
            expectedLen = DMA_MM_EXPECTED_SIZE(sizeof(Vp));
            break;
#ifdef	F3DEX_GBI_2
        case G_MV_MMTX:
        case G_MV_PMTX:
        case G_MV_MATRIX:
            expectedLen = DMA_MM_EXPECTED_SIZE(16);
            break;
        case G_MV_LIGHT:
            expectedLen = DMA_MM_EXPECTED_SIZE(sizeof(Light));
            break;
        case G_MV_POINT:
            // Not sure what to expect here
            expectedLen = DMA_MM_LEN(at);
            break;
        case G_MVO_LOOKATX:
        case G_MVO_LOOKATY:
            expectedLen = DMA_MM_EXPECTED_SIZE(sizeof(Light));
            break;
        case G_MVO_L0:
        case G_MVO_L1:
        case G_MVO_L2:
        case G_MVO_L3:
        case G_MVO_L4:
        case G_MVO_L5:
        case G_MVO_L6:
        case G_MVO_L7:
            expectedLen = DMA_MM_EXPECTED_SIZE(sizeof(Light));
            break;
#else
        case G_MV_LOOKATY:
        case G_MV_LOOKATX:
            expectedLen = sizeof(Light);
            break;
        case G_MV_L0:
        case G_MV_L1:
        case G_MV_L2:
        case G_MV_L3:
        case G_MV_L4:
        case G_MV_L5:
        case G_MV_L6:
        case G_MV_L7:
            expectedLen = sizeof(Light);
            break;
        case G_MV_TXTATT:
            // Not sure what to expect here
            expectedLen = DMA_MM_LEN(at);
            break;
        case G_MV_MATRIX_1:
        case G_MV_MATRIX_2:
        case G_MV_MATRIX_3:
        case G_MV_MATRIX_4:
            expectedLen = 16;
            break;
#endif
        default:
            sprintf(state->result->reasonMessage, "unrecognized copy target");
            return GFXValidatorInvalidArguments;
    }

    if (expectedLen != DMA_MM_LEN(at)) {
        sprintf(state->result->reasonMessage, "malformed copy size");
        return GFXValidatorInvalidArguments;
    }
    
    return gfxValidateAddress(state, at->dma.addr, 8);
}

enum GFXValidatorError gfxValidateVertex(struct GFXValidatorState* state, Gfx* at) {
    int vtxCount;
    int v0;
#ifdef F3DEX_GBI_2
    vtxCount = _SHIFTR(at->words.w0, 12, 8);
    v0 = _SHIFTR(at->words.w0, 1, 7) - vtxCount;
#else
    vtxCount = (DMA1_PARAM(at) >> 4) + 1;
    v0 = DMA1_PARAM(at) & 0xF;

    if (vtxCount * sizeof(Vtx) != DMA1_LEN(at)) {
        sprintf(state->result->reasonMessage, "malformed copy size");
        return GFXValidatorInvalidArguments;
    }
#endif

    if (vtxCount == 0) {
        sprintf(state->result->reasonMessage, "must include at least one vertex");
        return GFXValidatorInvalidArguments;
    }

    if (v0 + vtxCount > VERTEX_BUFFER_SIZE) {
        sprintf(state->result->reasonMessage, "vertex buffer overflow v0: %d n: %d", v0, vtxCount);
        return GFXValidatorInvalidArguments;
    }

    return gfxValidateAddress(state, at->dma.addr, 8);
}


enum GFXValidatorError gfxValidateDL(struct GFXValidatorState* state, Gfx* at) {
    if (DMA1_LEN(at) != 0) {
        sprintf(state->result->reasonMessage, "malformed length");
        return GFXValidatorInvalidArguments;
    } else if (DMA1_PARAM(at) != (DMA1_PARAM(at) & (G_DL_NOPUSH | G_DL_PUSH))) {
        sprintf(state->result->reasonMessage, "malformed flags");
        return GFXValidatorInvalidArguments;
    } else {
        return gfxValidateAddress(state, at->dma.addr, 8);
    }
}


enum GFXValidatorError gfxValidateSprite2DBase(struct GFXValidatorState* state, Gfx* at) {
    if (DMA1_LEN(at) != sizeof(uSprite) || DMA1_PARAM(at) != 0) {
        return GFXValidatorInvalidArguments;
    } else {
        return gfxValidateAddress(state, at->dma.addr, 8);
    }
}

enum GFXValidatorError gfxCheckTriangle(struct GFXValidatorState* state, int v0, int v1, int v2) {
    if (v0 >= MAX_VERTEX_VALUE || v1 >= MAX_VERTEX_VALUE || v2 >= MAX_VERTEX_VALUE) {
        sprintf(state->result->reasonMessage, "vertex index too large for vertex buffer");
        return GFXValidatorInvalidArguments;
    } else {
        return GFXValidatorErrorNone;
    }
} 

enum GFXValidatorError gfxValidateTri1(struct GFXValidatorState* state, Gfx* at) {
    int v0 = _SHIFTR(at->words.w1, 16, 8);
    int v1 = _SHIFTR(at->words.w1, 8, 8);
    int v2 = _SHIFTR(at->words.w1, 0, 8);

#ifdef F3DEX_GBI_2
    v0 = _SHIFTR(at->words.w0, 16, 8);
    v1 = _SHIFTR(at->words.w0, 8, 8);
    v2 = _SHIFTR(at->words.w0, 0, 8);
#else
    v0 = _SHIFTR(at->words.w1, 16, 8);
    v1 = _SHIFTR(at->words.w1, 8, 8);
    v2 = _SHIFTR(at->words.w1, 0, 8);
    flag = _SHIFTR(at->words.w1, 24, 8);
#endif

    return gfxCheckTriangle(state, v0, v1, v2);
}

enum GFXValidatorError gfxValidateTri2(struct GFXValidatorState* state, Gfx* at) {
    enum GFXValidatorError result = gfxCheckTriangle(
        state, 
        _SHIFTR(at->words.w0, 16, 8), 
        _SHIFTR(at->words.w0, 8, 8), 
        _SHIFTR(at->words.w0, 0, 8)
    );

    if (result != GFXValidatorErrorNone) {
        return result;
    }

    result = gfxCheckTriangle(
        state, 
        _SHIFTR(at->words.w1, 16, 8), 
        _SHIFTR(at->words.w1, 8, 8), 
        _SHIFTR(at->words.w1, 0, 8)
    );

    if (result != GFXValidatorErrorNone) {
        return result;
    }

    return GFXValidatorErrorNone;
}

enum GFXValidatorError gfxValidateCullDL(struct GFXValidatorState* state, Gfx* at) {
    return GFXValidatorErrorNone;
}

enum GFXValidatorError gfxValidatePopMtx(struct GFXValidatorState* state, Gfx* at) {
    // TODO handle G_SPRITE2D_DRAW in sprite mode
    int popCount;
#ifdef F3DEX_GBI_2
    popCount = at->words.w1 >> 6;
#else
    popCount = 1;
#endif

    if (state->matrixStackSize < popCount) {
        sprintf(state->result->reasonMessage, "matrix stack underflow");
        return GFXValidatorStackUnderflow;
#ifndef F3DEX_GBI_2
    } else if (at->words.w1 != G_MTX_MODELVIEW) {
        return GFXValidatorInvalidArguments;
#endif
    } else {
        state->matrixStackSize -= popCount;
        return GFXValidatorErrorNone;
    }
}


enum GFXValidatorError gfxValidateMoveWord(struct GFXValidatorState* state, Gfx* at) {
    int index = MOVE_WORD_IDX(at);
    int offset = MOVE_WORD_OFS(at);
    int data = MOVE_WORD_DATA(at);

    switch (index) {
        case G_MW_SEGMENT:
            if (offset > GFX_MAX_SEGMENTS * 4 || offset < 0) {
                sprintf(state->result->reasonMessage, "segment should be in the range [0, 15] got %d", offset >> 2);
                return GFXValidatorInvalidArguments;
            } else if (!gfxIsValidSegmentAddress(data)) {
                sprintf(state->result->reasonMessage, "invalid ram address for segment %08x", data);
                return GFXValidatorInvalidArguments;
            }

            state->segments[offset>>2] = data;
            break;
        case G_MW_CLIP:
            break;
        case G_MW_MATRIX:
            break;
#ifdef F3DEX_GBI_2
        case G_MW_FORCEMTX:
            break;
#else
        case G_MW_POINTS:
            break;
#endif // F3DEX_GBI_2
        case G_MW_NUMLIGHT:
            break;
        case G_MW_LIGHTCOL:
            break;
        case G_MW_FOG:
            break;
        case G_MW_PERSPNORM:
            break;
        default:
            return GFXValidatorInvalidArguments;
    }
    
    return GFXValidatorErrorNone;
}

enum GFXValidatorError gfxValidateTODO(struct GFXValidatorState* state, Gfx* at) {
    return GFXValidatorErrorNone;
}

enum GFXValidatorError gfxValidateList(struct GFXValidatorState* state, Gfx* gfx, Gfx* segmentGfx) {
    enum GFXValidatorError result = gfxPush(state, segmentGfx);

    if (result != GFXValidatorErrorNone) {
        return result;
    }

    int active = 1;
    int stackLocation = state->result->gfxStackSize-1;

    while (active) {
        int commandType = _SHIFTR(gfx->words.w0, 24, 8);

        if (commandType < 0 || commandType >= GFX_MAX_COMMAND_LEN) {
            result = GFXValidatorInvalidCommand;
            goto error;
        }

        CommandValidator validator = gfxCommandValidators[commandType];

        if (!validator) {
            sprintf(state->result->reasonMessage, "unrecongized command with id %08x", commandType);
            result = GFXValidatorInvalidCommand;
            goto error;
        }

        result = validator(state, gfx);

        if (result != GFXValidatorErrorNone) {
            goto error;
        }

        switch (commandType) {
            case (u8)G_ENDDL:
                active = 0;
                break;
            case (u8)G_DL:
                {
                    int next;
                    result = gfxTranslateAddress(state, gfx->dma.addr, &next);

                    if (result != GFXValidatorErrorNone) {
                        goto error;
                    }

                    next = PHYS_TO_K0(next);

                    if (gfx->dma.par == G_DL_NOPUSH) {
                        gfx = (Gfx*)next;
                    } else {
                        result = gfxValidateList(state, (Gfx*)next, (Gfx*)gfx->dma.addr);

                        if (result != GFXValidatorErrorNone) {
                            goto error;
                        }

                        ++gfx;
                    }
                }
                break;
            default:
                ++gfx;
                break;
        };
    }

    gfxPop(state);
    return GFXValidatorErrorNone;
error:
    state->result->gfxStack[stackLocation] = gfx;
    return result;
}

enum GFXValidatorError gfxValidate(OSTask* task, int maxGfxCount, struct GFXValidationResult* validateResult) {
    struct GFXValidatorState state;
    gfxInitState(&state, validateResult);
    
    if (task->t.type == M_GFXTASK) {
        enum GFXValidatorError result = gfxValidateList(&state, (Gfx*)task->t.data_ptr, (Gfx*)task->t.data_ptr);
        
        if (result != GFXValidatorErrorNone) {
            validateResult->reason = result;
            return result;
        }
    }

    return GFXValidatorErrorNone;
}

CommandValidator gfxCommandValidators[GFX_MAX_COMMAND_LEN] = {
    [G_SPNOOP] = gfxValidateNoop,
    [G_MTX] = gfxValidateMtx,
    [G_MOVEMEM] = gfxValidateMoveMem,
    [G_VTX] = gfxValidateVertex,
    [G_DL] = gfxValidateDL,
#ifdef G_SPRITE2D_BASE
    [G_SPRITE2D_BASE] = gfxValidateSprite2DBase,
#endif

    [(u8)G_TRI1] = gfxValidateTri1,
#ifdef G_TRI2
    [(u8)G_TRI2] = gfxValidateTri2,
#endif
    [(u8)G_CULLDL] = gfxValidateCullDL,
    [(u8)G_POPMTX] = gfxValidatePopMtx,
    [(u8)G_MOVEWORD] = gfxValidateMoveWord,

    [(u8)G_MODIFYVTX] = gfxValidateTODO,
    [(u8)G_BRANCH_Z] = gfxValidateTODO,
    [(u8)G_QUAD] = gfxValidateTODO,
    [(u8)G_SPECIAL_1] = gfxValidateTODO,
    [(u8)G_SPECIAL_2] = gfxValidateTODO,
    [(u8)G_SPECIAL_3] = gfxValidateTODO,
    [(u8)G_DMA_IO] = gfxValidateTODO,
    [(u8)G_TEXTURE] = gfxValidateTODO,
    [(u8)G_SETOTHERMODE_H] = gfxValidateTODO,
    [(u8)G_SETOTHERMODE_L] = gfxValidateTODO,
    [(u8)G_ENDDL] = gfxValidateTODO,
    [(u8)G_GEOMETRYMODE] = gfxValidateTODO,
#ifdef F3DEX_GBI_2
    [(u8)G_GEOMETRYMODE] = gfxValidateTODO,
#else
    [(u8)G_SETGEOMETRYMODE] = gfxValidateTODO,
    [(u8)G_CLEARGEOMETRYMODE] = gfxValidateTODO,
#endif
    [(u8)G_LINE3D] = gfxValidateTODO,
    [(u8)G_RDPHALF_1] = gfxValidateTODO,
    [(u8)G_RDPHALF_2] = gfxValidateTODO,
#ifdef F3DEX_GBI_2
    [(u8)G_TRI2] = gfxValidateTODO,
    [(u8)G_BRANCH_Z] = gfxValidateTODO,
    [(u8)G_LOAD_UCODE] = gfxValidateTODO,
#else
    [(u8)G_RDPHALF_CONT] = gfxValidateTODO,
#endif
    [(u8)G_LOAD_UCODE] = gfxValidateMoveWord,

    [(u8)G_NOOP] = gfxValidateTODO,

    [(u8)G_SETCIMG] = gfxValidateTODO,
    [(u8)G_SETZIMG] = gfxValidateTODO,
    [(u8)G_SETTIMG] = gfxValidateTODO,
    [(u8)G_SETCOMBINE] = gfxValidateTODO,
    [(u8)G_SETENVCOLOR] = gfxValidateTODO,
    [(u8)G_SETPRIMCOLOR] = gfxValidateTODO,
    [(u8)G_SETBLENDCOLOR] = gfxValidateTODO,
    [(u8)G_SETFOGCOLOR] = gfxValidateTODO,
    [(u8)G_SETFILLCOLOR] = gfxValidateTODO,
    [(u8)G_FILLRECT] = gfxValidateTODO,
    [(u8)G_SETTILE] = gfxValidateTODO,
    [(u8)G_LOADTILE] = gfxValidateTODO,
    [(u8)G_LOADBLOCK] = gfxValidateTODO,
    [(u8)G_SETTILESIZE] = gfxValidateTODO,
    [(u8)G_LOADTLUT] = gfxValidateTODO,
    [(u8)G_RDPSETOTHERMODE] = gfxValidateTODO,
    [(u8)G_SETPRIMDEPTH] = gfxValidateTODO,
    [(u8)G_SETSCISSOR] = gfxValidateTODO,
    [(u8)G_SETCONVERT] = gfxValidateTODO,
    [(u8)G_SETKEYR] = gfxValidateTODO,
    [(u8)G_SETKEYGB] = gfxValidateTODO,
    [(u8)G_RDPFULLSYNC] = gfxValidateTODO,
    [(u8)G_RDPTILESYNC] = gfxValidateTODO,
    [(u8)G_RDPPIPESYNC] = gfxValidateTODO,
    [(u8)G_RDPLOADSYNC] = gfxValidateTODO,
    [(u8)G_TEXRECTFLIP] = gfxValidateTODO,
    [(u8)G_TEXRECT] = gfxValidateTODO,
};