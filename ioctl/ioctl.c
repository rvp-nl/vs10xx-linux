/* ------------------------------------------------------------------------------------------------------------------------------
    vs10xx ioctl
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

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

#include "../module/vs10xx.h"

int main (int argc, char *argv[]) {

	struct vs10xx_scireg scireg;
	struct vs10xx_clockf clockf;
	struct vs10xx_volume volume;
	struct vs10xx_tone tone;
	struct vs10xx_info info;
	int fd, cmd, rc = 0;
	char* device;

	if ( (argc == 1) || (argc > 1 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")))) {

		printf("Usage: %s [options] [command] [parms ...]\n"
			"\nOptions:\n"
			"  -h --help\n"
			"  -d --device  device\n"
			"\nCommands:    Parameters:\n"
			"  reset\n"
			"  getscireg    regno\n"
			"  setscireg    regno msb lsb\n"
			"  getclockf\n"
			"  setclockf    mul add clk\n"
			"  getvolume\n"
			"  setvolume    left right\n"
			"  gettone\n"
			"  settone      tb tl bb bl\n"
			"  getinfo\n\n"
			, argv[0]
		);

		return 0;
	}

	if (argc > 2 && (!strcmp(argv[1], "-d") || !strcmp(argv[1], "--device"))) {

		device = argv[2];
		argc = argc - 3;
		cmd = 3;

	} else {

		device = "/dev/vs10xx-0";
		argc = argc - 1;
		cmd = 1;
	}

	fd = open(device, O_RDWR);

	if (fd == -1) {
		printf("error opening device: %s\n", device);
		return -1;
	}

	if (argc == 1 && !strcmp(argv[cmd],"reset")) {
		rc = ioctl (fd, VS10XX_CTL_RESET, NULL);
	}

	else if (argc == 2 && !strcmp(argv[cmd],"getscireg")) {
		scireg.reg = atoi(argv[cmd+1]);
		rc = ioctl (fd, VS10XX_CTL_GETSCIREG, &scireg);
		printf("scireg: %02X %02X%02X\n", scireg.reg, scireg.msb, scireg.lsb);
	}

	else if (argc == 4 && !strcmp(argv[cmd],"setscireg")) {
		scireg.reg = atoi(argv[cmd+1]);
		scireg.msb = atoi(argv[cmd+2]);
		scireg.lsb = atoi(argv[cmd+3]);
		rc = ioctl (fd, VS10XX_CTL_SETSCIREG, &scireg);
	}

	else if (argc == 1 && !strcmp(argv[cmd],"getclockf")) {
		rc = ioctl (fd, VS10XX_CTL_GETCLOCKF, &clockf);
		printf("clockf: mul:%u add:%u clk:%u\n", clockf.mul, clockf.add, clockf.clk);
	}

	else if (argc == 4 && !strcmp(argv[cmd],"setclockf")) {
		clockf.mul = atoi(argv[cmd+1]);
		clockf.add = atoi(argv[cmd+2]);
		clockf.clk = atoi(argv[cmd+3]);
		rc = ioctl (fd, VS10XX_CTL_SETCLOCKF, &clockf);
	}

	else if (argc == 1 && !strcmp(argv[cmd],"getvolume")) {
		rc = ioctl (fd, VS10XX_CTL_GETVOLUME, &volume);
		printf("volume: left:%u rght:%u\n", volume.left, volume.rght);
	}

	else if (argc == 3 && !strcmp(argv[cmd],"setvolume")) {
		volume.left = atoi(argv[cmd+1]);
		volume.rght = atoi(argv[cmd+2]);
		rc = ioctl (fd, VS10XX_CTL_SETVOLUME, &volume);
	}

	else if (argc == 1 && !strcmp(argv[cmd],"gettone")) {
		rc = ioctl (fd, VS10XX_CTL_GETTONE, &tone);
		printf("tone: treb-boost:%u treb-limit:%u bass-boost:%u bass-limit:%u\n",
			tone.trebboost, tone.treblimit, tone.bassboost, tone.basslimit);
	}

	else if (argc == 5 && !strcmp(argv[cmd],"settone")) {
		tone.trebboost = atoi(argv[cmd+1]);
		tone.treblimit = atoi(argv[cmd+2]);
		tone.bassboost = atoi(argv[cmd+3]);
		tone.basslimit = atoi(argv[cmd+4]);
		rc = ioctl (fd, VS10XX_CTL_SETTONE, &tone);
	}

	else if (argc == 1 && !strcmp(argv[cmd],"getinfo")) {
		rc = ioctl (fd, VS10XX_CTL_GETINFO, &info);
		printf("format: %d\n", info.fmt);
	}

	if (rc < 0) {
		printf("Error: %s\n", strerror(errno));
	}

	close (fd);

	return 0;
}

