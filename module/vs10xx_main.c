/* ------------------------------------------------------------------------------------------------------------------------------
    vs10xx main functions.
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
#include "vs10xx_device.h"
#include "vs10xx_queue.h"
#include "vs10xx_iocomm.h"

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/kthread.h>
#include <linux/delay.h>

static int debug = 0;
int *vs10xx_debug = &debug;
module_param(debug, int, 0644);

MODULE_DESCRIPTION("device driver for vs10xx");
MODULE_AUTHOR("Richard van Paasen");
MODULE_LICENSE("GPL v2");

static dev_t vs10xx_cdev_region;
static struct cdev *vs10xx_cdev;

/* ----------------------------------------------------------------------------------------------------------------------------- */
/* CHARDEV INTERFACE                                                                                                             */
/* ----------------------------------------------------------------------------------------------------------------------------- */

static int vs10xx_open(struct inode *inode, struct file *file) {

	int id = MINOR(inode->i_rdev);
	int status = 0;

	vs10xx_dbg("id:%d", id);

	status = vs10xx_device_open(id);

	if (status < 0) {
		vs10xx_inf("id:%d not valid or already open", id);
		status = -EACCES;
	} else {
		file->private_data = (void*)id;
	}

	return status;
}


static int vs10xx_release(struct inode *inode, struct file *file) {

	int id = (int)file->private_data;
	int status = 0;

	vs10xx_dbg("id:%d", id);

	status = vs10xx_device_release(id);

	if (status < 0) {
		status = -EIO;
	}

	return status;
}


static ssize_t vs10xx_write(struct file *file, const char __user *usrbuf, size_t lbuf, loff_t *ppos) {

	int id = (int)file->private_data;
	int status = 0;

	int nbytes = 0;
	int acttodo = 0;
	int copied = 0;

	struct vs10xx_queue_buf_t *buffer = NULL;

	vs10xx_nsy("id:%d", id);

	do {

		buffer = vs10xx_device_getbuf(id);

		if ((buffer == NULL)) {

			vs10xx_dbg("id:%d queue full", id);
			msleep(1);

		} else {

			acttodo = sizeof(buffer->data) > (lbuf-copied) ? (lbuf-copied) : sizeof(buffer->data);
			nbytes = acttodo - copy_from_user(buffer->data, usrbuf+copied, acttodo);
			buffer->len = nbytes;
			status = vs10xx_device_write(id);
			copied += nbytes;
		}

	} while (copied < lbuf && buffer != NULL);

	vs10xx_nsy("id:%d copied %d bytes", id, copied);

	return (status < 0 ? status : copied);
}


static long vs10xx_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {

	int ioctype =_IOC_TYPE(cmd), iocnr = _IOC_NR(cmd), /*iocdir = _IOC_DIR(cmd),*/ iocsize = _IOC_SIZE(cmd);
	int id = (int)file->private_data;
	char __user * usrbuf = (void __user *)arg;

	struct vs10xx_scireg scireg;
	struct vs10xx_clockf clockf;
	struct vs10xx_volume volume;
	struct vs10xx_tone tone;
	struct vs10xx_info info;

	if (ioctype != VS10XX_CTL_TYPE) {
		vs10xx_dbg("id:%d unsupported ioctl type:%c nr:%d", id, ioctype, iocnr);
		return -EINVAL;
	}

	switch (iocnr) {
		case _IOC_NR(VS10XX_CTL_RESET):
			vs10xx_device_reset(id);
			break;
		case _IOC_NR(VS10XX_CTL_GETSCIREG):
			copy_from_user(&scireg, usrbuf, iocsize);
			vs10xx_device_getscireg(id, &scireg);
			copy_to_user(usrbuf, &scireg, iocsize);
			break;
		case _IOC_NR(VS10XX_CTL_SETSCIREG):
			copy_from_user(&scireg, usrbuf, iocsize);
			vs10xx_device_setscireg(id, &scireg);
			break;
		case _IOC_NR(VS10XX_CTL_GETCLOCKF):
			vs10xx_device_getclockf(id, &clockf);
			copy_to_user(usrbuf, &clockf, iocsize);
			break;
		case _IOC_NR(VS10XX_CTL_SETCLOCKF):
			copy_from_user(&clockf, usrbuf, iocsize);
			vs10xx_device_setclockf(id, &clockf);
			break;
		case _IOC_NR(VS10XX_CTL_GETVOLUME):
			vs10xx_device_getvolume(id, &volume);
			copy_to_user(usrbuf, &volume, iocsize);
			break;
		case _IOC_NR(VS10XX_CTL_SETVOLUME):
			copy_from_user(&volume, usrbuf, iocsize);
			vs10xx_device_setvolume(id, &volume);
			break;
		case _IOC_NR(VS10XX_CTL_GETTONE):
			vs10xx_device_gettone(id, &tone);
			copy_to_user(usrbuf, &tone, iocsize);
			break;
		case _IOC_NR(VS10XX_CTL_SETTONE):
			copy_from_user(&tone, usrbuf, iocsize);
			vs10xx_device_settone(id, &tone);
			break;
		case _IOC_NR(VS10XX_CTL_GETINFO):
			vs10xx_device_getinfo(id, &info);
			copy_to_user(usrbuf, &info, iocsize);
			break;
		default:
			vs10xx_dbg("id:%d unsupported ioctl type:%c nr:%d", id, ioctype, iocnr);
			return -EINVAL;
	}

	return 0;
}

static const struct file_operations vs10xx_fops = {
	.owner = THIS_MODULE,
	.open = vs10xx_open,
	.release = vs10xx_release,
	.write = vs10xx_write,
	.unlocked_ioctl = vs10xx_ioctl,
};

/* ----------------------------------------------------------------------------------------------------------------------------- */
/* VS10XX SYSFS INTERFACE                                                                                                        */
/* ----------------------------------------------------------------------------------------------------------------------------- */

static struct class_attribute vs10xx_class_attrs[] = {
	__ATTR_NULL,
};

static struct class vs10xx_class = {
	.name =		VS10XX_NAME,
	.owner =	THIS_MODULE,
	.class_attrs =	vs10xx_class_attrs,
};

static ssize_t vs10xx_sys_reset_w(struct device *dev, struct device_attribute *attr, const char *buf, size_t size) {

	const int id = (int)dev_get_drvdata(dev);
	vs10xx_device_reset(id);
	return size;
}

static ssize_t vs10xx_sys_test_w(struct device *dev, struct device_attribute *attr, const char *buf, size_t size) {

	const int id = (int)dev_get_drvdata(dev);

	if (buf && (*buf=='M' || *buf=='m')) {
		vs10xx_device_memtest(id);
	}

	if (buf && (*buf=='S' || *buf=='s')) {
		vs10xx_device_sinetest(id);
	}

	return size;
}

static ssize_t vs10xx_sys_status_r(struct device *dev, struct device_attribute *attr, char *buf) {

	const int id = (int)dev_get_drvdata(dev);
	return vs10xx_device_status(id, buf);
}

static const DEVICE_ATTR(reset, 0222, NULL, vs10xx_sys_reset_w);
static const DEVICE_ATTR(test, 0666, NULL, vs10xx_sys_test_w);
static const DEVICE_ATTR(status, 0444, vs10xx_sys_status_r, NULL);

static const struct attribute *vs10xx_attrs[] = {
	&dev_attr_reset.attr,
	&dev_attr_test.attr,
	&dev_attr_status.attr,
	NULL,
};

static const struct attribute_group vs10xx_attr_group = {
	.attrs = (struct attribute **) vs10xx_attrs,
};

/* ----------------------------------------------------------------------------------------------------------------------------- */
/* MODULE INIT/EXIT                                                                                                              */
/* ----------------------------------------------------------------------------------------------------------------------------- */

static void vs10xx_cleanup (void) {

	int i;

	/* cleanup vs10xx devices */
	for (i=0; i<VS10XX_MAX_DEVICES; i++) {

		/* exit device */
		vs10xx_device_exit(i);

		/* destroy device */
		device_destroy(&vs10xx_class, MKDEV(MAJOR(vs10xx_cdev_region), i));

		/* exit queue */
		vs10xx_queue_exit(i);

		/* exit io */
		vs10xx_io_exit(i);
	}

	vs10xx_io_unregister();

	/* cleanup char device driver */
	if (vs10xx_cdev) {
		cdev_del(vs10xx_cdev);
	}

	/* unregister vs10xx class */
	class_unregister(&vs10xx_class);

	/* unregister vs10xx region */
	unregister_chrdev_region(vs10xx_cdev_region, VS10XX_MAX_DEVICES);

}

static int __init vs10xx_init (void) {

	int i, status = 0;
	struct device *dev;

	vs10xx_dbg("start");

	if (status == 0) {
		/* register vs10xx region */
		status = alloc_chrdev_region(&vs10xx_cdev_region, 0, VS10XX_MAX_DEVICES, VS10XX_NAME);
		if (status < 0) {
			vs10xx_err("alloc_chrdev_region");
		}
	}

	if (status == 0) {
		/* register vs10xx class */
		status = class_register(&vs10xx_class);
		if (status < 0) {
			vs10xx_err("class_register");
		}
	}

	if (status == 0) {
		/* create vs10xx character device driver */
		vs10xx_cdev = cdev_alloc();
		if (vs10xx_cdev == NULL) {
			vs10xx_err("cdev_alloc");
			status = -1;
		} else {
			cdev_init(vs10xx_cdev, &vs10xx_fops);
			status = cdev_add(vs10xx_cdev, vs10xx_cdev_region, VS10XX_MAX_DEVICES);
			if (status < 0) {
				vs10xx_err("cdev_add");
			}
		}
	}

	if (status == 0) {
		/* register io */
		status = vs10xx_io_register();
	}

	/* initialize vs10xx devices */
	for (i = 0; (status == 0) && (i < VS10XX_MAX_DEVICES); i++) {

		/* init io path */
		if (vs10xx_io_init(i) == 0) {

			/* init queue */
			status = vs10xx_queue_init(i);

			if (status == 0) {
				/* create vs10xx char device */
				dev = device_create(&vs10xx_class, NULL, MKDEV(MAJOR(vs10xx_cdev_region), i), (void*)i, "%s-%d", VS10XX_NAME, i);
				if (dev == NULL) {
					vs10xx_err("device_create id:%d name:%s-%d maj:%d min:%d", i, VS10XX_NAME, i, MAJOR(vs10xx_cdev_region), i);
					status = -1;
				} else {
					dev_set_drvdata(dev, (void*)i);
					status = sysfs_create_group(&dev->kobj, &vs10xx_attr_group);
				}
			}

			if (status == 0) {
				status = vs10xx_device_init(i, dev);
			}

			if (status == 0) {
				vs10xx_inf("id:%d name:%s-%d maj:%d min:%d", i, VS10XX_NAME, i, MAJOR(vs10xx_cdev_region), i);
			}
		}
	}

	if (status != 0) {

		vs10xx_cleanup();
	}

	vs10xx_dbg("done");

	return status;
}

static void __exit vs10xx_exit (void) {

	vs10xx_dbg("start");

	vs10xx_cleanup();

	vs10xx_dbg("done");
}

module_init(vs10xx_init);
module_exit(vs10xx_exit);

