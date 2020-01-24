/*
	Copyright (C) 2012 Lauri Kasanen
	Copyright (C) 2018 Genesis Cloud Ltd.

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

#ifndef RADEONTOP_H
#define RADEONTOP_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <locale.h>
#include <stdint.h>

enum {
	GRBM_STATUS = 0x8010,
	MMAP_SIZE = 0x14,
	VENDOR_AMD = 0x1002
};

// bits
struct radeontop_bits_t {
	unsigned int ee;
	unsigned int vgt;
	unsigned int gui;
	unsigned int ta;
	unsigned int tc;
	unsigned int sx;
	unsigned int sh;
	unsigned int spi;
	unsigned int smx;
	unsigned int sc;
	unsigned int pa;
	unsigned int db;
	unsigned int cb;
	unsigned int cr;
	uint64_t vram;
	uint64_t gtt;
	unsigned int sclk;
	unsigned int mclk;
};
typedef struct radeontop_bits_t radeontop_bits;

// function pointers to the right backend
typedef int (*radeontop_getgrbm_func)(uint32_t *out);
typedef int (*radeontop_getvram_func)(uint64_t *out);
typedef int (*radeontop_getgtt_func)(uint64_t *out);
typedef int (*radeontop_getsclk_func)(uint32_t *out);
typedef int (*radeontop_getmclk_func)(uint32_t *out);

// context.c
typedef void (*radeontop_die_func)(const char *why, void *userdata);

struct radeontop_context_t {
  radeontop_die_func die_func;
  void *die_userdata;
  int is_initialized;
  radeontop_bits *bits;
  uint64_t vramsize;
  uint64_t gttsize;
  unsigned int sclk_max; // kilohertz
  unsigned int mclk_max; // kilohertz

  // callbacks to the right backend
  radeontop_getgrbm_func getgrbm;
  radeontop_getvram_func getvram;
  radeontop_getgtt_func getgtt;
  radeontop_getsclk_func getsclk;
  radeontop_getmclk_func getmclk;

  // ticks
  radeontop_bits *results;
  pthread_mutex_t mutex;
};
typedef struct radeontop_context_t radeontop_context;

radeontop_context *radeontop_context_init(radeontop_die_func on_die, void *die_userdata);
radeontop_bits *radeontop_context_get_results(radeontop_context *context);
const char *radeontop_version();

// auth.c
void radeontop_authenticate_drm(int fd);

// detect.c
void radeontop_init_pci(radeontop_context   *context,
                        const char          *path,
                        short               *bus,
                        unsigned int        *device_id,
                        const unsigned char  forcemem);
int radeontop_get_family(radeontop_context *context, unsigned int id);
void radeontop_cleanup(radeontop_context *context);

// ticks.c
void radeontop_collect(radeontop_context *context,
                       unsigned int       ticks,
                       unsigned int       dumpinterval);

// ui.c
void radeontop_present(radeontop_context *context,
                       const unsigned int ticks,
                       const char card[],
                       unsigned int color,
                       const unsigned char bus,
                       const unsigned int dumpinterval);

// dump.c
void radeontop_dumpdata(radeontop_context *context,
                        const unsigned int ticks,
                        const char file[],
                        const unsigned int limit,
                        const unsigned char bus,
                        const unsigned int dumpinterval);

// chips
enum radeontop_radeon_family {
	UNKNOWN_CHIP,
	R600,
	RV610,
	RV630,
	RV670,
	RV620,
	RV635,
	RS780,
	RS880,
	RV770,
	RV730,
	RV710,
	RV740,
	CEDAR,
	REDWOOD,
	JUNIPER,
	CYPRESS,
	HEMLOCK,
	PALM,
	SUMO,
	SUMO2,
	BARTS,
	TURKS,
	CAICOS,
	CAYMAN,
	ARUBA,
	TAHITI,
	PITCAIRN,
	VERDE,
	OLAND,
	HAINAN,
	BONAIRE,
	KABINI,
	MULLINS,
	KAVERI,
	HAWAII,
	TOPAZ,
	TONGA,
	FIJI,
	CARRIZO,
	STONEY,
	POLARIS11,
	POLARIS10,
	POLARIS12,
	VEGAM,
	VEGA10,
	VEGA12,
	VEGA20,
	RAVEN,
	ARCTURUS,
	NAVI10,
	NAVI14,
	RENOIR,
	NAVI12
};

extern const char * const radeontop_family_str[];

// radeon.c
void radeontop_init_radeon(radeontop_context *context, int fd, int drm_major, int drm_minor);

// amdgpu.c
void radeontop_init_amdgpu(radeontop_context *context, int fd);
void radeontop_cleanup_amdgpu();

#endif
