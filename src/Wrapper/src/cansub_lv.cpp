#include "cansub_lv.h"

/* CCS CANsub device calls belong in this library. LabVIEW loads
   cansub_lv_wrapper.dll / libcansub_lv_wrapper.so through Call Library nodes. */

extern "C" int32_t cansub_lv_version(void) {
    return 0x01000000;
}
