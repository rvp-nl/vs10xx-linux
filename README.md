# vs10xx-linux
Linux kernel driver for the VLSI VS1053 and VS1063 audio dsp. It supports playback of aac, flac, mp3, ogg and wav files. The driver is optimized for performance. It uses pre-allocated queues and interrupt driven mechanism to ensure smooth playback at low resource usage. Up to four devices were used in a testbed to play mp3 simultaneously at low cpu load (10-15% on 200Mhz Atmel ARM 9).

Playing music can be done by simply 'cat mysong.mp3 > /dev/vs10xx-0'. A jukebox that plays music and outputs to a character device will do too. Volume and equalizer control is supported via ioctl.

Details on building and integrating the driver are provided in the doc/ folder.
