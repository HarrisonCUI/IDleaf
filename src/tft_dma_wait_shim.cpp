#include <Arduino.h>

#ifdef ATTRAX_TFT_DMA_WAIT_SHIM
#include <TFT_eSPI.h>

// The Seeed_GFX/TFT_eSPI S3 build used by Arduino CLI can reference dmaWait()
// even when the selected backend omits a definition in non-DMA builds.
void TFT_eSPI::dmaWait(void) {}
#endif
