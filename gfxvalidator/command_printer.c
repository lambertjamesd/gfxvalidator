
#include "command_printer.h"
#include "validator.h"
#include <string.h>

typedef int (*GFXValidatorPrinter)(unsigned long long command, char* output, unsigned maxOutputLength);

int gfxUnknownCommandPrinter(unsigned long long command, char* output, unsigned maxOutputLength) {
    return sprintf(output, "unknown 0x%08x%08x", (unsigned)(command >> 32), (unsigned)command);
}

GFXValidatorPrinter gfxCommandPrinters[GFX_MAX_COMMAND_LEN] = {
    
};

unsigned gfxPrintCommand(unsigned long long command, char* output, unsigned maxOutputLen) {
    unsigned commandType = (unsigned)(command >> 56) & 0xFF;

    GFXValidatorPrinter printer = gfxCommandPrinters[commandType];

    if (printer) {
        return printer(command, output, maxOutputLen);
    } else {
        return gfxUnknownCommandPrinter(command, output, maxOutputLen);
    }
}