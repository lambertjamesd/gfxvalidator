
#include "command_printer.h"
#include "validator.h"
#include <string.h>
#include "gfx_macros.h"

typedef int (*GFXValidatorPrinter)(Gfx command, char* output, unsigned maxOutputLength);

int gfxUnknownCommandPrinter(Gfx command, char* output, unsigned maxOutputLength) {
    return sprintf(output, "unknown 0x%08x%08x", (unsigned)(command.force_structure_alignment >> 32), (unsigned)command.force_structure_alignment);
}

int gfxDLCommandPrinter(Gfx command, char* output, unsigned maxOutputLength) {
    if (command.dma.par == G_DL_NOPUSH) {
        return sprintf(output, "gsSPBranchList(0x%08x)", command.words.w1);
    } else {
        return sprintf(output, "gsSPDisplayList(0x%08x)", command.words.w1);
    }
}

int gfxMtxCommandPrinter(Gfx command, char* output, unsigned maxOutputLength) { 
    int flags = DMA_MM_IDX(&command);

#ifdef F3DEX_GBI_2
    flags ^= G_MTX_PUSH;
#endif

    return sprintf(
        output, 
        "gsSPMatrix(0x%08x, %s | %s | %s)", 
        command.words.w1,
        (flags & G_MTX_PROJECTION) ? "G_MTX_PROJECTION" : "G_MTX_MODELVIEW",
        (flags & G_MTX_LOAD) ? "G_MTX_LOAD" : "G_MTX_MUL",
        (flags & G_MTX_PUSH) ? "G_MTX_PUSH" : "G_MTX_NOPUSH"
    );
}

int gfxMoveMemCommandPrinter(Gfx command, char* output, unsigned maxOutputLength) { 
    int location = DMA_MM_IDX(&command);

    switch (location) {
        case G_MV_VIEWPORT:
            return sprintf(
                output, 
                "gsSPViewport(0x%08x)", 
                command.words.w1
            );
#ifdef	F3DEX_GBI_2
        case G_MV_MATRIX:
            return sprintf(
                output, 
                "gsSPForceMatrix(0x%08x)", 
                command.words.w1
            );
        case G_MV_LIGHT:
            return sprintf(
                output, 
                "gsSPLight(0x%08x, %d)", 
                command.words.w1,
                (DMA_MM_OFS(&command) - 24) / 24
            );
        case G_MVO_LOOKATX:
            return sprintf(
                output, 
                "gsSPLookAtX(0x%08x)", 
                command.words.w1
            );
        case G_MVO_LOOKATY:
            return sprintf(
                output, 
                "gsSPLookAtY(0x%08x)", 
                command.words.w1
            );
#else
        // TODO Old gfx
#endif
        default:
        return sprintf(
            output, 
            "gsDma2p(G_MOVEMEM, 0x%08x, *, 0x%x, *)", 
            command.words.w1,
            location
        );
    }
}

int gfxVtxCommandPrinter(Gfx command, char* output, unsigned maxOutputLength) { 
    int vtxCount;
    int v0;
#ifdef F3DEX_GBI_2
    vtxCount = _SHIFTR(command.words.w0, 12, 8);
    v0 = _SHIFTR(command.words.w0, 1, 7) - vtxCount;
#else
    vtxCount = (DMA1_PARAM(&command) >> 4) + 1;
    v0 = DMA1_PARAM(&command) & 0xF;
#endif
    return sprintf(
        output, 
        "gsSPVertex(0x%08x, %d, %d)", 
        command.words.w1,
        vtxCount,
        v0
    );
}

int gfxTri1CommandPrinter(Gfx command, char* output, unsigned maxOutputLength) { 
#ifdef F3DEX_GBI_2
    int v0 = _SHIFTR(command.words.w0, 16, 8);
    int v1 = _SHIFTR(command.words.w0, 8, 8);
    int v2 = _SHIFTR(command.words.w0, 0, 8);
#else
    int v0 = _SHIFTR(command.words.w1, 16, 8);
    int v1 = _SHIFTR(command.words.w1, 8, 8);
    int v2 = _SHIFTR(command.words.w1, 0, 8);
#endif

    return sprintf(
        output, 
        "gsSP1Triangle(%d, %d, %d, 0)", 
        command.words.w1,
        v0,
        v1,
        v2
    );
}

int gfxTri2CommandPrinter(Gfx command, char* output, unsigned maxOutputLength) { 
    return sprintf(
        output, 
        "gsSP2Triangles(%d, %d, %d, 0, %d, %d, %d, 0)", 
        command.words.w1,
        _SHIFTR(command.words.w0, 16, 8), 
        _SHIFTR(command.words.w0, 8, 8), 
        _SHIFTR(command.words.w0, 0, 8),
        _SHIFTR(command.words.w1, 16, 8), 
        _SHIFTR(command.words.w1, 8, 8), 
        _SHIFTR(command.words.w1, 0, 8)
    );
}

int gfxPopMtxCommandPrinter(Gfx command, char* output, unsigned maxOutputLength) { 
    int popCount;
#ifdef F3DEX_GBI_2
    popCount = command.words.w1 >> 6;
#else
    popCount = 1;
#endif

    if (popCount == 1) {
        return sprintf(
            output, 
            "gsSPPopMatrix(G_MTX_MODELVIEW)"
        );
    }

    return sprintf(
        output, 
        "gsSPPopMatrixN(G_MTX_MODELVIEW, %d)",
        popCount
    );
}

int gfxMoveWordCommandPrinter(Gfx command, char* output, unsigned maxOutputLength) { 
    int index = MOVE_WORD_IDX(&command);
    int offset = MOVE_WORD_OFS(&command);
    int data = MOVE_WORD_DATA(&command);

    switch (index) {
        case G_MW_SEGMENT:
            return sprintf(
                output, 
                "gsSPPopMatrixN(0x%x, 0x%08x)",
                offset >> 2,
                command.words.w1
            );
        case G_MW_CLIP:
            return sprintf(
                output, 
                "gsSPClipRatio(*)"
            );
        case G_MW_MATRIX:
            return sprintf(
                output, 
                "gsSPInsertMatrix(*)"
            );
#ifdef F3DEX_GBI_2
        case G_MW_FORCEMTX:
            return sprintf(
                output, 
                "gsSPInsertMatrix(*)"
            );
#else
        case G_MW_POINTS:
            return sprintf(
                output, 
                "gsSPModifyVertex(*)"
            );
#endif // F3DEX_GBI_2
        case G_MW_NUMLIGHT:
            return sprintf(
                output, 
                "gsSPNumLights(%d)",
                data / 24
            );
        case G_MW_LIGHTCOL:
            return sprintf(
                output, 
                "gsSPLightColor(*, %x)",
                data
            );
        case G_MW_FOG:
            return sprintf(
                output, 
                "gsSPFog*(*)"
            );
        case G_MW_PERSPNORM:
            return sprintf(
                output, 
                "gsSPPerspNormalize(%d)",
                data
            );
        default:
            return sprintf(
                output, 
                "gsMoveWd(%d, %d, %d)",
                index,
                offset,
                data
            );
    }
}

GFXValidatorPrinter gfxCommandPrinters[GFX_MAX_COMMAND_LEN] = {
    [G_DL] = gfxDLCommandPrinter,
    [G_MTX] = gfxMtxCommandPrinter,
    [G_MOVEMEM] = gfxMoveMemCommandPrinter,
    [G_VTX] = gfxVtxCommandPrinter,
    [G_TRI1] = gfxTri1CommandPrinter,
    [G_TRI2] = gfxTri2CommandPrinter,
    [G_POPMTX] = gfxPopMtxCommandPrinter,
    [G_MOVEWORD] = gfxMoveWordCommandPrinter,
};

unsigned gfxPrintCommand(Gfx command, char* output, unsigned maxOutputLen) {
    unsigned commandType = (unsigned)(command.force_structure_alignment >> 56) & 0xFF;

    GFXValidatorPrinter printer = gfxCommandPrinters[commandType];

    if (printer) {
        return printer(command, output, maxOutputLen);
    } else {
        return gfxUnknownCommandPrinter(command, output, maxOutputLen);
    }
}