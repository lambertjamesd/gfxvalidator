
#include "command_printer.h"
#include <string.h>

typedef int (*GFXValidatorPrinter)(unsigned long long command, char* output, unsigned maxOutputLength);

int gfxUnknownCommandPrinter(unsigned long long command, char* output, unsigned maxOutputLength) {
    return snprintf(output, maxOutputLength, "unknown 0x%08d%08d", (unsigned)(command >> 32), (unsigned)command);
}

GFXValidatorPrinter gfxCommandPrinters[MAX_COMMAND_LEN] = {
    
};

unsigned gfxPrintCommand(unsigned long long command, char* output, unsigned maxOutputLen) {
    unsigned commandType = (unsigned)(command >> 56) & 0xFF;

    GFXValidatorPrinter printer = gfxCommandPrinters[commandType];

    if (printer) {
        return printer(command, output, maxOutputLength);
    } else {
        return gfxUnknownCommandPrinter(command, output, maxOutputLength);
    }
}