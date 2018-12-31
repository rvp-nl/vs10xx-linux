/* ------------------------------------------------------------------------------------------------------------------------------
    vs10xx io functions.
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

#ifndef VS10XX_IOCOMM_H
#define VS10XX_IOCOMM_H

int vs10xx_io_register(void);
void vs10xx_io_unregister(void);

int vs10xx_io_init(int id);
void vs10xx_io_exit(int id);

void vs10xx_io_reset(int id);
int vs10xx_io_isready(int id);
int vs10xx_io_wtready(int id, unsigned timeout);

int vs10xx_io_ctrl_xf(int id, const char *txbuf, unsigned txlen, char *rxbuf, unsigned rxlen);
int vs10xx_io_data_rx(int id, char *rxbuf, unsigned rxlen);
int vs10xx_io_data_tx(int id, const char *txbuf, unsigned txlen);

#endif
