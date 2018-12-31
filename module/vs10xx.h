/* ------------------------------------------------------------------------------------------------------------------------------
    vs10xx bsp and ioctl functions.
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

#ifndef VS10XX_H
#define VS10XX_H

/* ----------------------------------------------------------------------------------------------------------------------------- */
/* Board Definition                                                                                                              */
/* ----------------------------------------------------------------------------------------------------------------------------- */
#define VS10XX_MAX_DEVICES 4

#define VS10XX_SPI_CTRL "vs10xx-ctrl"
#define VS10XX_SPI_DATA "vs10xx-data"

struct vs10xx_board_info {
	int device_id;
	int gpio_reset;
	int gpio_dreq;
};

/* ----------------------------------------------------------------------------------------------------------------------------- */
/* IOCTLS                                                                                                                        */
/* ----------------------------------------------------------------------------------------------------------------------------- */

#define VS10XX_CTL_TYPE      'Q'
#define VS10XX_CTL_RESET     _IO(VS10XX_CTL_TYPE, 1)

#define VS10XX_CTL_GETSCIREG _IOWR(VS10XX_CTL_TYPE, 10, struct vs10xx_scireg)
#define VS10XX_CTL_SETSCIREG _IOWR(VS10XX_CTL_TYPE, 11, struct vs10xx_scireg)
#define VS10XX_CTL_GETCLOCKF _IOR(VS10XX_CTL_TYPE, 12, struct vs10xx_clockf)
#define VS10XX_CTL_SETCLOCKF _IOW(VS10XX_CTL_TYPE, 13, struct vs10xx_clockf)
#define VS10XX_CTL_GETVOLUME _IOR(VS10XX_CTL_TYPE, 14, struct vs10xx_volume)
#define VS10XX_CTL_SETVOLUME _IOW(VS10XX_CTL_TYPE, 15, struct vs10xx_volume)
#define VS10XX_CTL_GETTONE   _IOR(VS10XX_CTL_TYPE, 16, struct vs10xx_tone)
#define VS10XX_CTL_SETTONE   _IOW(VS10XX_CTL_TYPE, 17, struct vs10xx_tone)
#define VS10XX_CTL_GETINFO   _IOR(VS10XX_CTL_TYPE, 18, struct vs10xx_info)

struct vs10xx_scireg {
	unsigned char reg; /* 0..15  */
	unsigned char msb; /* 0..255 */
	unsigned char lsb; /* 0..255 */
};

struct vs10xx_clockf {
	unsigned int mul; /* 0..7    fixed clock multiplication factor */
	unsigned int add; /* 0..3    additional multiplication factor  */
	unsigned int clk; /* 0..2048 input clock (XTALI) in 4kHz steps */
};

struct vs10xx_volume {
	unsigned int left; /* 0..255 */
	unsigned int rght; /* 0..255 */
};

struct vs10xx_tone {
	unsigned int trebboost; /* 0..15 (x 1.5 dB) amount of boost for high frequencies       */
	unsigned int treblimit; /* 0..15 (x 1  kHz) frequency above which amplitude is boosted */
	unsigned int bassboost; /* 0..15 (x 1   dB) amount of boost for low frequencies        */
	unsigned int basslimit; /* 0..15 (x 10  Hz) frequency below which amplitude is boosted */
};

typedef enum {
	VS10XX_FMT_NUL=0, VS10XX_FMT_WAV=1, VS10XX_FMT_AAC=2, VS10XX_FMT_WMA=3,
	VS10XX_FMT_MID=4, VS10XX_FMT_OGG=5, VS10XX_FMT_MP3=6, VS10XX_FMT_FLC=7
} vs10xx_fmt_t;

struct vs10xx_info {
	vs10xx_fmt_t fmt;
};

#endif

