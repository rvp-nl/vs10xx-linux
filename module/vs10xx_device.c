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

#include "vs10xx_common.h"
#include "vs10xx_iocomm.h"
#include "vs10xx_queue.h"
#include "vs10xx_device.h"
#include "vs10xx_plugins.h"

#include <linux/delay.h>
#include <linux/kthread.h>

/* clockf value */
static int clockf = 0xc000;
module_param(clockf, int, 0644);

/* plugin to load */
static int plugin = 2;
module_param(plugin, int, 0644);

/* wait [ms] on close */
static int wclose = 0;
module_param(wclose, int, 0644);

struct vs10xx_device_t {
	int id;
	int open;
	int valid;
	int start;
	int finish;
	struct mutex lock;
	struct device *dev;
	struct task_struct *kthread;
	wait_queue_head_t wq;
	unsigned long underrun;
	int version;
};

static struct vs10xx_device_t vs10xx_device[VS10XX_MAX_DEVICES];

/* ----------------------------------------------------------------------------------------------------------------------------- */
/* VS10XX DEVICE READ/WRITE SCI REGISTERS                                                                                        */
/* ----------------------------------------------------------------------------------------------------------------------------- */

static int vs10xx_device_w_sci_reg(int id, unsigned char reg, unsigned char msb, unsigned char lsb) {

	int status = 0;

	unsigned char cmd[] = {0x02, reg, msb, lsb};

	if (status == 0) {

		status = vs10xx_io_ctrl_xf(id, cmd, sizeof(cmd), NULL, 0);

		vs10xx_nsy("id:%d %02X %02X%02X", id, (int)cmd[1], (int)cmd[2], (int)cmd[3]);

		if (!vs10xx_io_wtready(id, 10)) {

			vs10xx_err("id:%d timeout (reg=%x)", id, reg);

			status = -1;
		}
	}

	return status;
}

static int vs10xx_device_r_sci_reg(int id, unsigned char reg, unsigned char* msb, unsigned char* lsb) {

	int status = 0;

	unsigned char cmd[] = {0x03, reg};
	unsigned char res[] = {0x00, 0x00};

	if (status == 0) {

		status = vs10xx_io_ctrl_xf(id, cmd, sizeof(cmd), res, sizeof(res));

		*msb = res[0];
		*lsb = res[1];

		vs10xx_nsy("id:%d %02X %02X%02X", id, (int)cmd[1], (int)res[0], (int)res[1]);

		if (!vs10xx_io_wtready(id, 10)) {

			vs10xx_err("id:%d timeout (reg=%x)", id, reg);
			status = -1;
		}
	}

	return status;
}


/* ----------------------------------------------------------------------------------------------------------------------------- */
/* VS10XX DEVICE LOAD PLUGIN                                                                                                     */
/* ----------------------------------------------------------------------------------------------------------------------------- */

static int vs10xx_device_plugin(int id, const unsigned short* vs10xx_plugin, int n) {

	int i = 0, status = 0;

	vs10xx_dbg("start:%d", id);

	if (vs10xx_plugin) {

		/* load a compressed plugin */
		while (status == 0 && i < n) {
			unsigned short addr, m, val;
			addr = vs10xx_plugin[i++];
			m = vs10xx_plugin[i++];
			if (m & 0x8000U) {
				/* RLE run */
				m &= 0x7FFF;
				val = vs10xx_plugin[i++];
				while (status == 0 && m--) {
					/* replicate m samples */
					status = vs10xx_device_w_sci_reg(id, addr, (val >> 8) & 0x00ff, val & 0x00ff);
				}
			} else {
				/* Copy run */
				while (status == 0 && m--) {
					 /* copy m samples */
					val = vs10xx_plugin[i++];
					status = vs10xx_device_w_sci_reg(id, addr, (val >> 8) & 0x00ff, val & 0x00ff);
				}
			}
		}

		if (i < n) {

			vs10xx_err("id:%d plugin incomplete (%d/%d bytes)", id, i, n);

			status = -1;

		} else {

			if (status == 0) {

				vs10xx_inf("id:%d plugin loaded (%d/%d bytes)", id, i, n);

			} else {

				vs10xx_err("id:%d plugin failed", id);
			}

		}

	}

	vs10xx_dbg("done:%d", id);

	return status;
}

/* ----------------------------------------------------------------------------------------------------------------------------- */
/* VS10XX DEVICE RESET                                                                                                           */
/* ----------------------------------------------------------------------------------------------------------------------------- */

static void vs10xx_device_hwreset(int id) {

	vs10xx_dbg("start:%d", id);

	vs10xx_io_reset(id);

	if (!vs10xx_io_wtready(id, 50)) {

		vs10xx_wrn("id:%d timeout", id);
	}

	vs10xx_dbg("done:%d", id);
}

static int vs10xx_device_swreset(int id, int testmode) {

	int status = 0;

	vs10xx_dbg("start:%d", id);

	if (testmode) {

		// WRITE MODE SM_SDINEW | SM_RESET | SM_TESTS
		status = vs10xx_device_w_sci_reg(id, 0x00, 0x08, 0x24);

	} else {

		// WRITE MODE SM_SDINEW | !SM_RESET | SM_TESTS
		status = vs10xx_device_w_sci_reg(id, 0x00, 0x08, 0x04);
	}

	if (status == 0 && clockf != 0) {

		// WRITE CLOCKF
		status = vs10xx_device_w_sci_reg(id, 0x03, (clockf & 0xff00) >> 8, (clockf & 0xff));
	}

	vs10xx_dbg("done:%d", id);

	return status;
}

/* ----------------------------------------------------------------------------------------------------------------------------- */
/* VS10XX DEVICE OPERATIONS                                                                                                      */
/* ----------------------------------------------------------------------------------------------------------------------------- */

int vs10xx_device_reset(int id) {

	int status = 0;

	vs10xx_dbg("start:%d", id);

	mutex_lock(&vs10xx_device[id].lock);

	vs10xx_queue_flush(id);

	vs10xx_device_hwreset(id);

	status = vs10xx_device_swreset(id, 0);

	if (status != 0) {

		vs10xx_device_hwreset(id);
		status = vs10xx_device_swreset(id, 0);
	}

	if (status == 0) {

		unsigned char msb, lsb;
		status = vs10xx_device_r_sci_reg(id, 0x01, &msb, &lsb);
		vs10xx_device[id].version = (lsb & 0xf0) >> 4;
	}

	if (status == 0) {

		switch (vs10xx_device[id].version) {
			case 4:
				vs10xx_inf("id:%d found vs1053 device", id);
				if (plugin) {
					status = vs10xx_device_plugin(id, vs1053_plugin, sizeof(vs1053_plugin)/sizeof(vs1053_plugin[0]));
				}
				break;
			case 6:
				vs10xx_inf("id:%d found vs1063 device", id);
				if (plugin) {
					status = vs10xx_device_plugin(id, vs1063_plugin, sizeof(vs1063_plugin)/sizeof(vs1063_plugin[0]));
				}
				break;
			default:
				vs10xx_err("id:%d unsupported device (vrs=%d)", id, vs10xx_device[id].version);
				status = -1;
				break;
		}
	}

	if (status == 0) {

		unsigned char buffer[2] = { 0, 0 };
		status = vs10xx_io_data_tx(id, buffer, 2);
	}

	mutex_unlock(&vs10xx_device[id].lock);

	vs10xx_dbg("done:%d", id);

	return status;
}


int vs10xx_device_sinetest(int id) {

	int status = 0;

	vs10xx_inf("id:%d started sine test", id);

	mutex_lock(&vs10xx_device[id].lock);

	vs10xx_queue_flush(id);

	vs10xx_device_hwreset(id);

	status = vs10xx_device_swreset(id, 1);

	if (status != 0) {

		vs10xx_device_hwreset(id);
		status = vs10xx_device_swreset(id, 1);
	}

	if (status == 0) {

		// SET VOLUME TO 75%
		status = vs10xx_device_w_sci_reg(id, 0x0B, 64, 64);
	}

	if (status == 0) {

		// GENERATE SINE
		unsigned char test[] = {0x53, 0xef, 0x6e, 0xa3, 0x00, 0x00, 0x00, 0x00};
		status = vs10xx_io_data_tx(id, test, sizeof(test));
	}

	mutex_unlock(&vs10xx_device[id].lock);

	return status;
}


int vs10xx_device_memtest(int id) {

	int status = 0;
	unsigned char msb, lsb;

	vs10xx_inf("id:%d started mem test", id);

	mutex_lock(&vs10xx_device[id].lock);

	vs10xx_queue_flush(id);

	vs10xx_device_hwreset(id);

	status = vs10xx_device_swreset(id, 1);

	if (status != 0) {

		vs10xx_device_hwreset(id);
		status = vs10xx_device_swreset(id, 0);
	}

	if (status == 0) {

		// TEST MEMORY
		unsigned char test[] = {0x4d, 0xea, 0x6d, 0x54, 0x00, 0x00, 0x00, 0x00};
		status = vs10xx_io_data_tx(id, test, sizeof(test));
	}

	if (status == 0) {

		msleep(VS10XX_MSEC(1100000));

		// READ HDAT0
		status = vs10xx_device_r_sci_reg(id, 0x08, &msb, &lsb);
		msb = msb & 0x83; /* bits 14:10 are unsued */
	}

	mutex_unlock(&vs10xx_device[id].lock);

	if (status == 0) {

		vs10xx_inf("id:%d mem test complete - result:%02X%02X", id, (int)msb, (int)lsb);
	}

	return status;
}


int vs10xx_device_getscireg(int id, struct vs10xx_scireg *scireg) {

	int status = 0;

	mutex_lock(&vs10xx_device[id].lock);

	status = vs10xx_device_r_sci_reg(id, scireg->reg & 0x0F, &scireg->msb, &scireg->lsb);

	mutex_unlock(&vs10xx_device[id].lock);

	return status;
}


int vs10xx_device_setscireg(int id, struct vs10xx_scireg *scireg) {

	int status = 0;

	mutex_lock(&vs10xx_device[id].lock);

	status = vs10xx_device_w_sci_reg(id, scireg->reg & 0x0F, scireg->msb, scireg->lsb);

	mutex_unlock(&vs10xx_device[id].lock);

	return status;
}


int vs10xx_device_getclockf(int id, struct vs10xx_clockf *clkf) {

	int status = 0;
	unsigned char msb, lsb;

	mutex_lock(&vs10xx_device[id].lock);

	status = vs10xx_device_r_sci_reg(id, 0x03, &msb, &lsb);

	mutex_unlock(&vs10xx_device[id].lock);

	clkf->mul = (msb >> 5) & 0x07;
	clkf->add = (msb >> 3) & 0x03;
	clkf->clk = (msb & 0x07) << 8 | lsb;

	return status;
}


int vs10xx_device_setclockf(int id, struct vs10xx_clockf *clkf) {

	int status = 0;
	unsigned char msb = ((clkf->mul & 0x07) << 5) | ((clkf->add & 0x03) << 3) | ((clkf->clk >> 8) & 0x07);
	unsigned char lsb = clkf->clk & 0xff;

	mutex_lock(&vs10xx_device[id].lock);

	status = vs10xx_device_w_sci_reg(id, 0x03, msb, lsb);

	mutex_unlock(&vs10xx_device[id].lock);

	return status;
}


int vs10xx_device_getvolume(int id, struct vs10xx_volume *volume) {

	int status = 0;
	unsigned char msb, lsb;

	mutex_lock(&vs10xx_device[id].lock);

	status = vs10xx_device_r_sci_reg(id, 0x0B, &msb, &lsb);

	mutex_unlock(&vs10xx_device[id].lock);

	volume->left = 255 - msb;
	volume->rght = 255 - lsb;

	return status;
}


int vs10xx_device_setvolume(int id, struct vs10xx_volume *volume) {

	int status = 0;
	unsigned char msb = 255 - (volume->left & 0xff);
	unsigned char lsb = 255 - (volume->rght & 0xff);

	mutex_lock(&vs10xx_device[id].lock);

	status = vs10xx_device_w_sci_reg(id, 0x0B, msb, lsb);

	mutex_unlock(&vs10xx_device[id].lock);

	return status;
}


int vs10xx_device_gettone(int id, struct vs10xx_tone *tone) {

	int status = 0;
	unsigned char msb, lsb;

	mutex_lock(&vs10xx_device[id].lock);

	status = vs10xx_device_r_sci_reg(id, 0x02, &msb, &lsb);

	mutex_unlock(&vs10xx_device[id].lock);

	tone->trebboost = (msb >> 4) & 0x0f;
	tone->treblimit = msb & 0x0f;
	tone->bassboost = (lsb >> 4) & 0x0f;
	tone->basslimit = lsb & 0x0f;

	return status;
}


int vs10xx_device_settone(int id, struct vs10xx_tone *tone) {

	int status = 0;
	unsigned char msb = ((tone->trebboost & 0x0f) << 4) | (tone->treblimit & 0x0f);
	unsigned char lsb = ((tone->bassboost & 0x0f) << 4) | (tone->basslimit & 0x0f);

	mutex_lock(&vs10xx_device[id].lock);

	status = vs10xx_device_w_sci_reg(id, 0x02, msb, lsb);

	mutex_unlock(&vs10xx_device[id].lock);

	return status;
}

int vs10xx_device_getinfo(int id, struct vs10xx_info *info) {

	int status = 0;
	unsigned char msb=0, lsb=0;

	mutex_lock(&vs10xx_device[id].lock);

	status = vs10xx_device_r_sci_reg(id, 0x09, &msb, &lsb);

	mutex_unlock(&vs10xx_device[id].lock);

	if (msb==0x76 && lsb==0x65) info->fmt = VS10XX_FMT_WAV;
	else if (msb==0x41 && lsb==0x54) info->fmt = VS10XX_FMT_AAC;
	else if (msb==0x41 && lsb==0x44) info->fmt = VS10XX_FMT_AAC;
	else if (msb==0x4D && lsb==0x34) info->fmt = VS10XX_FMT_AAC;
	else if (msb==0x57 && lsb==0x4D) info->fmt = VS10XX_FMT_WMA;
	else if (msb==0x4D && lsb==0x54) info->fmt = VS10XX_FMT_MID;
	else if (msb==0x4F && lsb==0x67) info->fmt = VS10XX_FMT_OGG;
	else if (msb==0x66 && lsb==0x4C) info->fmt = VS10XX_FMT_FLC;
	else if (msb==0xFF && lsb>=0xE0) info->fmt = VS10XX_FMT_MP3;
	else info->fmt = VS10XX_FMT_NUL;

	return status;
}

/* ----------------------------------------------------------------------------------------------------------------------------- */
/* VS10XX DEVICE THREAD                                                                                                          */
/* ----------------------------------------------------------------------------------------------------------------------------- */

static inline void vs10xx_device_setopen(int id) {

	mutex_lock(&vs10xx_device[id].lock);
	vs10xx_device[id].open = 1;
	mutex_unlock(&vs10xx_device[id].lock);
}

static inline void vs10xx_device_clropen(int id) {

	mutex_lock(&vs10xx_device[id].lock);
	vs10xx_device[id].open = 0;
	mutex_unlock(&vs10xx_device[id].lock);

	wake_up(&vs10xx_device[id].wq);
}

static inline int vs10xx_device_getopen(int id) {

	return vs10xx_device[id].open;
}

static inline void vs10xx_device_setpause(int id) {

	mutex_lock(&vs10xx_device[id].lock);
	vs10xx_device[id].start = 0;
	mutex_unlock(&vs10xx_device[id].lock);
}

static inline void vs10xx_device_clrpause(int id) {

	mutex_lock(&vs10xx_device[id].lock);
	vs10xx_device[id].start = 1;
	mutex_unlock(&vs10xx_device[id].lock);

	wake_up(&vs10xx_device[id].wq);
}

static inline int vs10xx_device_getpause(int id) {

	return (vs10xx_device[id].start ? 0 : 1);
}

static inline void vs10xx_device_setfinish(int id) {

	mutex_lock(&vs10xx_device[id].lock);
	vs10xx_device[id].finish = 1;
	mutex_unlock(&vs10xx_device[id].lock);
}

static inline void vs10xx_device_clrfinish(int id) {

	mutex_lock(&vs10xx_device[id].lock);
	vs10xx_device[id].finish = 0;
	mutex_unlock(&vs10xx_device[id].lock);

	wake_up(&vs10xx_device[id].wq);
}

static inline int vs10xx_device_getfinish(int id) {

	return vs10xx_device[id].finish;
}


static int vs10xx_device_kthread(void *arg) {

	struct vs10xx_device_t *device = (struct vs10xx_device_t*)arg;

	do {

		int status = 0;

		if (vs10xx_device_getpause(device->id)) {

			/* on hold, wait for start */
			wait_event_timeout(device->wq, device->start, msecs_to_jiffies(10));

		}
		else if (vs10xx_queue_isempty(device->id)) {

			/* no data --> pause */
			if (vs10xx_device_getopen(device->id)) {

				vs10xx_nsy("id:%d no data", device->id);
				vs10xx_device_setpause(device->id);

				if (!vs10xx_device_getfinish(device->id)) {

					/* log underrun */
					device->underrun++;
				}
			}

		} else if (!vs10xx_io_wtready(device->id, 10)) {

			/* io not ready to receive */
			vs10xx_nsy("id:%d not ready", device->id);

		} else {

			mutex_lock(&device->lock);

			while (vs10xx_io_isready(device->id) && !vs10xx_queue_isempty(device->id)) {

				/* transmit data */
				struct vs10xx_queue_buf_t *buffer;
				buffer = vs10xx_queue_gethead(device->id);
				status = vs10xx_io_data_tx(device->id, buffer->data, buffer->len);
				if (status == 0) {
					vs10xx_queue_dequeue(device->id);
				}
			}

			mutex_unlock(&device->lock);
		}

	} while (!kthread_should_stop());

	return 0;
}

/* ----------------------------------------------------------------------------------------------------------------------------- */
/* VS10XX DEV INTERFACE                                                                                                          */
/* ----------------------------------------------------------------------------------------------------------------------------- */

int vs10xx_device_isvalid(int id) {

	return !(id < 0 || id >= VS10XX_MAX_DEVICES || !vs10xx_device[id].valid);
}

int vs10xx_device_open(int id) {

	int status = 0;

	if (vs10xx_device_getopen(id)) {

		status = -1;

	} else {

		vs10xx_device_setopen(id);
	}

	return status;
}

int vs10xx_device_release(int id) {

	struct vs10xx_scireg scireg_addr = { 0x07, 0x1e, 0x06 };
	struct vs10xx_scireg scireg_data = { 0x06, 0x00, 0x00 };
	struct vs10xx_queue_buf_t buffer;
	struct vs10xx_info info;
	unsigned char msb, lsb;
	int i = wclose, status = 0;

	while ((i-- > 0) && !vs10xx_queue_isempty(id)) {

		msleep(1);
	}

	if (!vs10xx_queue_isempty(id)) {

		vs10xx_dbg("id:%d queue not empty, closing anyway", id);
	}

	mutex_lock(&vs10xx_device[id].lock);
	vs10xx_queue_flush(id);
	mutex_unlock(&vs10xx_device[id].lock);

	vs10xx_device_setpause(id);

	// get stream type and endfillbytes
	vs10xx_io_wtready(id, 250);
	vs10xx_device_getinfo(id, &info);
	status = vs10xx_device_setscireg(id, &scireg_addr);
	status = vs10xx_device_getscireg(id, &scireg_data);

	buffer.len = sizeof(buffer.data);
	memset(buffer.data, scireg_data.lsb, buffer.len);

	// send endfillbytes to end file
	// bufsize = 32 => send 384 (FLAC) of 65 (other) buffers
	for (i = (info.fmt==VS10XX_FMT_FLC?384:65); i > 0; i--) {

		vs10xx_io_wtready(id, 10);

		status = vs10xx_io_data_tx(id, buffer.data, buffer.len);
	}

	// set SM CANCEL
	status = vs10xx_device_w_sci_reg(id, 0x00, 0x08, 0x08);

	// send endfillbytes awaiting cancel confimation
	// bufsize = 32 => send 64 buffers
	for (i = 0; i < 64; i++) {

		status = vs10xx_io_data_tx(id, buffer.data, buffer.len);

		vs10xx_io_wtready(id, 10);

		status = vs10xx_device_r_sci_reg(id, 0x00, &msb, &lsb);

		if ((lsb & 0x08) == 0) {

			break;
		}
	}

	if ((lsb & 0x08) == 1) {

		vs10xx_device_swreset(id, 0);
		vs10xx_wrn("id:%d device did not cancel, swreset issued!", id);
	}

	vs10xx_device_clropen(id);

	return status;
}

struct vs10xx_queue_buf_t* vs10xx_device_getbuf(int id) {

	struct vs10xx_queue_buf_t *buffer = NULL;

	if (!vs10xx_queue_isfull(id)) {
		mutex_lock(&vs10xx_device[id].lock);
		buffer = vs10xx_queue_getslot(id);
		mutex_unlock(&vs10xx_device[id].lock);
	}

	return buffer;
}

int vs10xx_device_write(int id) {

	int status = 0;

	mutex_lock(&vs10xx_device[id].lock);
	vs10xx_queue_enqueue(id);
	mutex_unlock(&vs10xx_device[id].lock);

	if (vs10xx_device_getpause(id) && vs10xx_queue_isfull(id)) {

		vs10xx_device_clrpause(id);
	}

	return status;
}

int vs10xx_device_getfree(int id) {

	return vs10xx_queue_getfree(id);
}

int vs10xx_device_status(int id, char* buf) {

	struct vs10xx_info info;

	vs10xx_device_getinfo(id, &info);

	return sprintf(buf, "xversion: %d\nplaystat: %s\nstrmtype: %d\ndreq/rdy: %d\nunderrun: %lu\n",
		vs10xx_device[id].version,
		vs10xx_device[id].start ? (vs10xx_device[id].finish ? "F" : "P") : "S",
		info.fmt,
		vs10xx_io_isready(id),
		vs10xx_device[id].underrun
	);
}

/* ----------------------------------------------------------------------------------------------------------------------------- */
/* VS10XX DEVICE INIT/EXIT                                                                                                       */
/* ----------------------------------------------------------------------------------------------------------------------------- */

int vs10xx_device_init(int id, struct device *dev) {

	int status = 0;

	vs10xx_dbg("start:%d", id);

	vs10xx_device[id].id = id;
	vs10xx_device[id].open = 0;
	vs10xx_device[id].valid = 0;
	vs10xx_device[id].start = 0;
	vs10xx_device[id].finish = 0;
	vs10xx_device[id].underrun = 0;
	vs10xx_device[id].version = -1;

	vs10xx_device[id].dev = dev;

	/* initialize mutex */
	mutex_init(&vs10xx_device[id].lock);

	/* initialize wait queue */
	init_waitqueue_head(&vs10xx_device[id].wq);

	/* reset device */
	status = vs10xx_device_reset(id);

	if (status == 0) {

		/* start device thread */
		vs10xx_device[id].kthread = kthread_run(vs10xx_device_kthread, &vs10xx_device[id], "%s-%d", VS10XX_NAME, id);

		if (vs10xx_device[id].kthread == NULL) {

			vs10xx_err("id:%d kthread_run", id);
			status = -1;
		}
	}

	if (status == 0) {

		/* flag valid */
		vs10xx_device[id].valid = 1;
	}

	vs10xx_dbg("done:%d", id);

	return status;
}

void vs10xx_device_exit(int id) {

	if (vs10xx_device_isvalid(id)) {

		/* reset device */
		vs10xx_device_hwreset(id);

		if (vs10xx_device[id].kthread != NULL) {

			/* stop device thread */
			kthread_stop(vs10xx_device[id].kthread);
		}
	}

	vs10xx_device[id].valid = 0;
}

