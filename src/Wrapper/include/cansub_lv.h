#pragma once

#include <stdint.h>

#if defined(_WIN32)
#  if defined(CANSUB_LV_BUILD)
#    define CANSUB_LV_API __declspec(dllexport)
#  else
#    define CANSUB_LV_API __declspec(dllimport)
#  endif
#else
#  define CANSUB_LV_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Packed version: 0xMMmmppbb for major.minor.patch.build. */
CANSUB_LV_API int32_t cansub_lv_version(void);

#ifdef __cplusplus
}
#endif
