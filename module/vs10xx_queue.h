/* ------------------------------------------------------------------------------------------------------------------------------
    vs10xx queue functions.
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

#ifndef VS10XX_QUEUE_H
#define VS10XX_QUEUE_H

struct vs10xx_queue_buf_t {
// up to 32 bytes can be written
// without checking for dreq
	char data[32];
	unsigned len;
};

int vs10xx_queue_init(int id);
void vs10xx_queue_exit(int id);
void vs10xx_queue_flush(int id);

int vs10xx_queue_isfull(int id);
int vs10xx_queue_isempty(int id);
int vs10xx_queue_getfree(int id);

struct vs10xx_queue_buf_t* vs10xx_queue_getslot(int id);
struct vs10xx_queue_buf_t* vs10xx_queue_gethead(int id);

void vs10xx_queue_enqueue(int id);
void vs10xx_queue_dequeue(int id);

#endif
