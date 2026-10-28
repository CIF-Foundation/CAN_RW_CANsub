#include "cansub_lv.h"

#include <stdio.h>

int main(void) {
    const int32_t version = cansub_lv_version();
    printf("cansub_lv_wrapper %d.%d.%d.%d\n",
           (version >> 24) & 0xFF,
           (version >> 16) & 0xFF,
           (version >> 8) & 0xFF,
           version & 0xFF);
    return version == 0 ? 1 : 0;
}
