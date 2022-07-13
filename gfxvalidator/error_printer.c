
#include "./command_printer.h"
#include "./validator.h"
#include <string.h>

#define TMP_BUFFER_SIZE 64

typedef unsigned (*ErrorPrinter)(struct GFXValidationResult* result, char* output, unsigned maxOutputLen);

unsigned gfxUnknownError(struct GFXValidationResult* result, char* output, unsigned maxOutputLen) {
    return sprintf(output, "unknown error %d", result->reason);
}

ErrorPrinter gfxErrorPrinters[GFXValidatorErrorCount] = {

};

void gfxGenerateReadableMessage(struct GFXValidationResult* result, gfxPrinter printer) {
    char tmpBuffer[TMP_BUFFER_SIZE];

    if (result->reason == GFXValidatorErrorNone) {
        printer(tmpBuffer, sprintf(tmpBuffer, "success"));
        return;
    }

    for (int i = 0; i < result->gfxStackSize; ++i) {
        char* curr = tmpBuffer;
        unsigned currOffset = 0;
        currOffset += sprintf(curr + currOffset, "0x%08x: ", (unsigned)result->gfxStack[i]);
        currOffset += gfxPrintCommand(result->gfxStack[i]->force_structure_alignment, curr + currOffset, (unsigned)(TMP_BUFFER_SIZE - currOffset));

        if (currOffset < TMP_BUFFER_SIZE) {
            curr[currOffset++] = '\n';
        }

        printer(tmpBuffer, currOffset);
    }

    ErrorPrinter errorPrinter = 0;

    if (result->reason < GFXValidatorErrorCount) {
        errorPrinter = gfxErrorPrinters[result->reason];
    }

    if (!errorPrinter) {
        errorPrinter = gfxUnknownError;
    }

    printer(tmpBuffer, errorPrinter(result, tmpBuffer, TMP_BUFFER_SIZE));
}