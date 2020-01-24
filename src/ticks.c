/*
    Copyright (C) 2012 Lauri Kasanen

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "radeontop.h"
#include <pthread.h>

struct collector_args_t {
	unsigned int ticks;
	unsigned int dumpinterval;

  radeontop_context *context;
  radeontop_getgrbm_func getgrbm;
  radeontop_getsclk_func getsclk;
  radeontop_getmclk_func getmclk;
  radeontop_getvram_func getvram;
  radeontop_getgtt_func getgtt;
};

static void *collector(void *arg) {
	struct collector_args_t *args = (struct collector_args_t *) arg;
  radeontop_context *context = args->context;

	const unsigned int ticks = args->ticks;
	const unsigned int dumpinterval = args->dumpinterval;

	radeontop_bits res[2];

	// Save one second's worth of history
	radeontop_bits *history = calloc(ticks * dumpinterval, sizeof(radeontop_bits));
	unsigned int cur = 0, curres = 0;

	const useconds_t sleeptime = 1e6 / ticks;

	while (1) {
		unsigned int stat;
		args->getgrbm(&stat);

		memset(&history[cur], 0, sizeof(radeontop_bits));

    pthread_mutex_lock(&context->mutex);
		if (stat & context->bits.ee) history[cur].ee = 1;
		if (stat & context->bits.vgt) history[cur].vgt = 1;
		if (stat & context->bits.gui) history[cur].gui = 1;
		if (stat & context->bits.ta) history[cur].ta = 1;
		if (stat & context->bits.tc) history[cur].tc = 1;
		if (stat & context->bits.sx) history[cur].sx = 1;
		if (stat & context->bits.sh) history[cur].sh = 1;
		if (stat & context->bits.spi) history[cur].spi = 1;
		if (stat & context->bits.smx) history[cur].smx = 1;
		if (stat & context->bits.sc) history[cur].sc = 1;
		if (stat & context->bits.pa) history[cur].pa = 1;
		if (stat & context->bits.db) history[cur].db = 1;
		if (stat & context->bits.cr) history[cur].cr = 1;
		if (stat & context->bits.cb) history[cur].cb = 1;
    pthread_mutex_unlock(&context->mutex);
		args->getsclk(&history[cur].sclk);
		args->getmclk(&history[cur].mclk);

		usleep(sleeptime);
		cur++;
		cur %= ticks * dumpinterval;

		// One second has passed, we have one sec's worth of data
		if (cur == 0) {
			unsigned int i;

			memset(&res[curres], 0, sizeof(radeontop_bits));

			for (i = 0; i < ticks * dumpinterval; i++) {
				res[curres].ee += history[i].ee;
				res[curres].vgt += history[i].vgt;
				res[curres].gui += history[i].gui;
				res[curres].ta += history[i].ta;
				res[curres].tc += history[i].tc;
				res[curres].sx += history[i].sx;
				res[curres].sh += history[i].sh;
				res[curres].spi += history[i].spi;
				res[curres].smx += history[i].smx;
				res[curres].sc += history[i].sc;
				res[curres].pa += history[i].pa;
				res[curres].db += history[i].db;
				res[curres].cb += history[i].cb;
				res[curres].cr += history[i].cr;
				res[curres].mclk += history[i].mclk;
				res[curres].sclk += history[i].sclk;
			}

			args->getvram(&res[curres].vram);
			args->getgtt(&res[curres].gtt);

			// Atomically write it to the pointer
      pthread_mutex_lock(&context->mutex);
			context->results = &res[curres];
      pthread_mutex_unlock(&context->mutex);

			curres++;
			curres %= 2;
		}
	}

	return NULL;
}

void radeontop_collect(radeontop_context *context,
                       unsigned int       ticks,
                       unsigned int       dumpinterval) {

	// Start a thread collecting data
	pthread_t tid;
	pthread_attr_t attr;

	// We don't care to join this thread
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	struct collector_args_t *args = malloc(sizeof(*args));
	args->ticks = ticks;
	args->dumpinterval = dumpinterval;
  args->context = context;
  args->getgrbm = context->getgrbm;
  args->getsclk = context->getsclk;
  args->getmclk = context->getmclk;
  args->getvram = context->getvram;
  args->getgtt = context->getgtt;

	pthread_create(&tid, &attr, collector, args);
}
