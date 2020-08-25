#ifndef HIFBDEV_H
#define HIFBDEV_H

#include "lv_drv_conf.h"
#include "lv_hal_disp.h"

#ifdef __cplusplus
extern "C" {
#endif

int hifbdev_init(void);
void hifbdev_exit(void);
void hifbdev_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p);

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
