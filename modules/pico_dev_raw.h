/*********************************************************************
   PicoTCP. Copyright (c) 2012-2017 Altran Intelligent Systems. Some rights reserved.
   See COPYING, LICENSE.GPLv2 and LICENSE.GPLv3 for usage.

 *********************************************************************/
#ifndef INCLUDE_PICO_RAW
#define INCLUDE_PICO_RAW
#include "pico_config.h"
#include "pico_device.h"

void pico_raw_destroy(struct pico_device *raw);
struct pico_device *pico_raw_create(char *name, char *ifname);

#endif

