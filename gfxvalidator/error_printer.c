
#include "./command_printer.h"
#include "./validator.h"
#include <string.h>

#define TMP_BUFFER_SIZE 64

typedef unsigned (*ErrorPrinter)(struct GFXValidationResult* result, char* output, unsigned maxOutputLen);

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

        curr[currOffset] = '\0';

        printer(tmpBuffer, currOffset);
    }

    if (result->reasonMessage[0]) {
        printer(result->reasonMessage, strlen(result->reasonMessage));
    }
}