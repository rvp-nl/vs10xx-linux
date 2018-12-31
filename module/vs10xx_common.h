/* ------------------------------------------------------------------------------------------------------------------------------
    vs10xx common definitions.
    Copyright (C) 2010-2013 Richard van Paasen <rvp-nl@t3i.nl>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
   ----------------------------------------------------------------------------------------------------------------------------- */

#ifndef VS10XX_COMMON_H
#define VS10XX_COMMON_H

#include "vs10xx.h"

#include <linux/list.h>
#include <linux/device.h>
#include <linux/kdev_t.h>

#define VS10XX_NAME "vs10xx"

#define VS10XX_KHZ 12288
#define VS10XX_USEC(x) ( (((x)*1000) / VS10XX_KHZ) + 1)
#define VS10XX_MSEC(x) ( ((x) / VS10XX_KHZ) + 1)

#define MIN(a,b) ((a)<(b) ? (a) : (b))
#define MAX(a,b) ((a)>(b) ? (a) : (b))

#define vs10xx_printk(level, fmt, arg...) \
        printk(level "%s: " fmt "\n", __func__, ## arg)

#define vs10xx_err(fmt, arg...) \
        vs10xx_printk(KERN_ERR, fmt, ## arg)

#define vs10xx_wrn(fmt, arg...) \
        vs10xx_printk(KERN_WARNING, fmt, ## arg)

#define vs10xx_inf(fmt, arg...) \
        vs10xx_printk(KERN_INFO, fmt, ## arg)

#define vs10xx_dbg(fmt, arg...)                     \
	do {                                            \
		if (*vs10xx_debug > 0)                      \
			vs10xx_printk(KERN_DEBUG, fmt, ## arg); \
        } while (0)

#define vs10xx_nsy(fmt, arg...)                     \
	do {                                            \
		if (*vs10xx_debug > 1)                      \
			vs10xx_printk(KERN_DEBUG, fmt, ## arg); \
        } while (0)

extern int *vs10xx_debug;

#endif

