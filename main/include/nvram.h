#ifndef _NVRAM_H_
#define _NVRAM_H_

#include "nvs_flash.h"
extern nvs_handle nvram_handler;

esp_err_t nvram_init();
#endif