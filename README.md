
# GFX validator

Validates a libultra display list giving a human readable error if there is a problem

## Usage

```C
void graphicsOutputMessageSerial(char* message, unsigned len) {
    // unfloader
    usb_write(DATATYPE_TEXT, message, len);
}

OSTask scTask;

// generate task

struct GFXValidationResult validationResult;

if (gfxValidate(&scTask->list, MAX_DL_LENGTH, &validationResult) != GFXValidatorErrorNone) {
    gfxGenerateReadableMessage(&validationResult, graphicsOutputMessageSerial);
}
```

## TODO

calling gSPVertex with G_LIGHTING set and no lights 

aligned color buffers and z buffers