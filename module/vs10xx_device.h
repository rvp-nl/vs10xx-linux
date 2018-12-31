/* ------------------------------------------------------------------------------------------------------------------------------
    vs10xx device functions.
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

#ifndef VS10XX_DEVICE_H
#define VS10XX_DEVICE_H

int vs10xx_device_init(int id, struct device *dev);
void vs10xx_device_exit(int id);

int vs10xx_device_open(int id);
int vs10xx_device_release(int id);
int vs10xx_device_write(int id);

int vs10xx_device_isvalid(int id);
int vs10xx_device_status(int id, char* buf);

struct vs10xx_queue_buf_t* vs10xx_device_getbuf(int id);
int vs10xx_device_getfree(int id);

int vs10xx_device_reset(int id);

int vs10xx_device_sinetest(int id);
int vs10xx_device_memtest(int id);

int vs10xx_device_getscireg(int id, struct vs10xx_scireg *scireg);
int vs10xx_device_setscireg(int id, struct vs10xx_scireg *scireg);

int vs10xx_device_getclockf(int id, struct vs10xx_clockf *clkf);
int vs10xx_device_setclockf(int id, struct vs10xx_clockf *clkf);

int vs10xx_device_getvolume(int id, struct vs10xx_volume *volume);
int vs10xx_device_setvolume(int id, struct vs10xx_volume *volume);
int vs10xx_device_gettone(int id, struct vs10xx_tone *tone);
int vs10xx_device_settone(int id, struct vs10xx_tone *tone);

int vs10xx_device_getinfo(int id, struct vs10xx_info *info);

#endif
