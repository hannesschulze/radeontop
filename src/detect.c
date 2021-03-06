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

#include "radeontop.h"
#include "gettext.h"
#include <pciaccess.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <xf86drm.h>
#include <errno.h>
#include <pthread.h>

const void *area;

static int find_pci(radeontop_context *context,
                    short              bus,
                    struct pci_device *pci_dev) {
	int ret = pci_system_init();
	if (ret)
    context->die_func (_("Failed to init pciaccess"), context->die_userdata);

	struct pci_device *dev;
	struct pci_id_match match;

	match.vendor_id = VENDOR_AMD;
	match.device_id = PCI_MATCH_ANY;
	match.subvendor_id = PCI_MATCH_ANY;
	match.subdevice_id = PCI_MATCH_ANY;
	match.device_class = 0;
	match.device_class_mask = 0;
	match.match_data = 0;

	struct pci_device_iterator *iter = pci_id_match_iterator_create(&match);

	while ((dev = pci_device_next(iter))) {
		pci_device_probe(dev);
		if ((dev->device_class & 0x00ffff00) != 0x00030000 &&
			(dev->device_class & 0x00ffff00) != 0x00038000)
			continue;
		if (bus < 0 || bus == dev->bus) {
			*pci_dev = *dev;
			break;
		}
	}

	pci_iterator_destroy(iter);
	pci_system_cleanup();
	return (dev == NULL);
}

static int getgrbm_pci(uint32_t *out) {
	*out = *(uint32_t *)((const char *) area + 0x10);
	return 0;
}

static void open_pci(radeontop_context *context, struct pci_device *gpu_device) {
	// Warning: gpu_device is a copy of a freed struct. Do not access any pointers!
	int reg = 2;
	if (radeontop_get_family(context, gpu_device->device_id) >= BONAIRE)
		reg = 5;

	if (!gpu_device->regions[reg].size)
    context->die_func(_("Can't get the register area size"), context->die_userdata);

//	printf("Found area %p, size %lu\n", area, dev->regions[reg].size);

	int mem = open("/dev/mem", O_RDONLY);
	if (mem < 0)
    context->die_func(_("Cannot access GPU registers, are you root?"), context->die_userdata);

	area = mmap(NULL, MMAP_SIZE, PROT_READ, MAP_PRIVATE, mem,
			gpu_device->regions[reg].base_addr + 0x8000);
	if (area == MAP_FAILED)
    context->die_func(_("mmap failed"), context->die_userdata);

	context->getgrbm = getgrbm_pci;
}

static void cleanup_pci() {
	munmap((void *) area, MMAP_SIZE);
}

static int init_drm(radeontop_context *context, int drm_fd) {
	drmVersionPtr ver = drmGetVersion(drm_fd);

	if (!ver) {
		perror(_("Failed to query driver version"));
		close(drm_fd);
		return -1;
	}

	radeontop_authenticate_drm(drm_fd);

	if (strcmp(ver->name, "radeon") == 0)
		radeontop_init_radeon(context, drm_fd, ver->version_major, ver->version_minor);
	else if (strcmp(ver->name, "amdgpu") == 0)
#ifdef ENABLE_AMDGPU
		radeontop_init_amdgpu(context, drm_fd);
#else
		fprintf(stderr, _("amdgpu support is not compiled in (libdrm 2.4.63 required)\n"));
#endif
	else {
		fprintf(stderr, _("Unsupported driver %s\n"), ver->name);
		close(drm_fd);
		drm_fd = -1;
	}

/*	printf("Version %u.%u.%u, name %s\n",
		ver->version_major,
		ver->version_minor,
		ver->version_patchlevel,
		ver->name);*/

	drmFreeVersion(ver);
	return drm_fd;
}

static void open_drm_bus(radeontop_context *context, const struct pci_device *dev) {
	char busid[32];
	snprintf(busid, sizeof(busid), "pci:%04x:%02x:%02x.%u",
			 dev->domain, dev->bus, dev->dev, dev->func);

	int fd = drmOpen(NULL, busid);

	if (fd >= 0)
		init_drm(context, fd);
	else
		printf(_("Failed to open DRM node, no VRAM support.\n"));
}

static int open_drm_path(radeontop_context *context, const char *path) {
	int fd = open(path, O_RDWR);

	if (fd >= 0)
		fd = init_drm(context, fd);
	else
		fprintf(stderr, _("Failed to open %s: %s\n"),
			path, strerror(errno));

	return fd;
}

#ifdef DRM_DEVICE_GET_PCI_REVISION
#define DRMGETDEVICE(fd, dev)	drmGetDevice2(fd, 0, dev)
#define DRMGETDEVICES(dev, max)	drmGetDevices2(0, dev, max)
#else
#define DRMGETDEVICE(fd, dev)	drmGetDevice(fd, dev)
#define DRMGETDEVICES(dev, max)	drmGetDevices(dev, max)
#endif

#ifdef DRM_BUS_PCI
static int find_drm(radeontop_context *context,
                    short              bus,
                    short             *device_bus,
                    unsigned int      *device_id) {
	drmDevicePtr *devs;
	int count, i, j, fd = -1;

	count = DRMGETDEVICES(NULL, 0);

	if (count <= 0) {
		if (count < 0)
			drmError(-count, _("Failed to find DRM devices"));
		return 1;
	}

	if (!(devs = calloc(count, sizeof(drmDevicePtr))))
		context->die_func(_("Failed to allocate memory for DRM\n"), context->die_userdata);

	if ((count = DRMGETDEVICES(devs, count)) < 0) {
		drmError(-count, _("Failed to get DRM devices"));
		return 1;
	}

	for (i = 0; i < count; i++) {
		if (devs[i]->bustype != DRM_BUS_PCI ||
			devs[i]->deviceinfo.pci->vendor_id != VENDOR_AMD ||
			(bus >= 0 && bus != devs[i]->businfo.pci->bus))
			continue;

		// try render node first, as it does not require to drop master
		for (j = DRM_NODE_MAX - 1; j >= 0; j--) {
			if (!(1 << j & devs[i]->available_nodes))
				continue;
			if ((fd = open_drm_path(context, devs[i]->nodes[j])) < 0)
				continue;

			*device_bus = devs[i]->businfo.pci->bus;
			*device_id = devs[i]->deviceinfo.pci->device_id;
			break;
		}

		if (fd >= 0)
			break;
	}

	drmFreeDevices(devs, count);
	free(devs);
	return (fd < 0);
}
#endif

#ifdef HAS_DRMGETDEVICE
static void device_info_drm(int fd, short *bus, unsigned int *device_id) {
	drmDevicePtr dev;
	int err;

	if ((err = DRMGETDEVICE(fd, &dev))) {
		drmError(err, _("Failed to get device info"));
		return;
	}

	if (dev->bustype != DRM_BUS_PCI) {
		fprintf(stderr, _("Unsupported bus type %d\n"),
			dev->bustype);
		return;
	}

	*bus = dev->businfo.pci->bus;
	*device_id = dev->deviceinfo.pci->device_id;
	drmFreeDevice(&dev);
}
#endif

// do-nothing backend used as fallback
#define UNUSED(v)	(void) v
static int getuint32_null(uint32_t *out) { UNUSED(out); return -1; }
static int getuint64_null(uint64_t *out) { UNUSED(out); return -1; }

void radeontop_init_pci(radeontop_context   *context,
                        const char          *path,
                        short               *bus,
                        unsigned int        *device_id,
                        const unsigned char  forcemem) {
	short device_bus = -1;
	int err = 1;
	context->getgrbm = context->getsclk = context->getmclk = getuint32_null;
	context->getvram = context->getgtt = getuint64_null;

	if (path) {
		int fd = open_drm_path(context, path);

		if (fd < 0 || context->getgrbm == getuint32_null)
			exit(1);

#ifdef HAS_DRMGETDEVICE
		device_info_drm(fd, &device_bus, device_id);
#else
		fprintf(stderr, _("DRM device detection is not compiled in (libdrm 2.4.66 required)\n"));
#endif
		err = 0;
	}

	// If a path was not specified, search and open the first AMD
	// video card, picking the correct PCI bus if provided.
#ifdef DRM_BUS_PCI
	if (!forcemem && err)
		err = find_drm(context, *bus, &device_bus, device_id);
#endif

	// This is the fallback method for older libdrm that doesn't
	// have drmGetDevices, operating systems where drmGetDevices
	// is not implemented, older radeon kernel driver without GRBM
	// readings, AMD Catalyst driver or when no driver is loaded
	if (context->getgrbm == getuint32_null) {
		struct pci_device pci_dev;
		memset(&pci_dev, 0, sizeof(struct pci_device));
		err = find_pci(context, *bus, &pci_dev);

		if (!err) {
			// DRM support for VRAM
			open_drm_bus(context, &pci_dev);

			if (forcemem)
				printf(_("Forcing the /dev/mem path.\n"));

			if (forcemem || context->getgrbm == getuint32_null)
				open_pci(context, &pci_dev);

			device_bus = pci_dev.bus;
			*device_id = pci_dev.device_id;
		}
	}

	if (err)
		context->die_func(_("Can't find Radeon cards"), context->die_userdata);

  pthread_mutex_lock(&context->mutex);
	context->bits->vram = (context->getvram != getuint64_null);
	context->bits->gtt = (context->getgtt != getuint64_null);
  pthread_mutex_unlock(&context->mutex);
	*bus = device_bus;
  context->is_initialized = 1;
}

void radeontop_cleanup(radeontop_context *context) {
  if (context->is_initialized) {
	  cleanup_pci();

#ifdef ENABLE_AMDGPU
	  radeontop_cleanup_amdgpu();
#endif
  }

  free(context->bits);
  free(context);
}

static void init_bits(radeontop_context *context, int fam) {
  pthread_mutex_lock(&context->mutex);

	// The majority of these is the same from R600 to Southern Islands.
	context->bits->ee = (1U << 10);
	context->bits->vgt = (1U << 16) | (1U << 17);
	context->bits->ta = (1U << 14);
	context->bits->tc = (1U << 19);
	context->bits->sx = (1U << 20);
	context->bits->sh = (1U << 21);
	context->bits->spi = (1U << 22);
	context->bits->smx = (1U << 23);
	context->bits->sc = (1U << 24);
	context->bits->pa = (1U << 25);
	context->bits->db = (1U << 26);
	context->bits->cr = (1U << 27);
	context->bits->cb = (1U << 30);
	context->bits->gui = (1U << 31);

	// R600 has a different texture bit, and only R600 has the TC, CR, SMX bits
	if (fam < RV770) {
		context->bits->ta = (1U << 18);
	} else {
		context->bits->tc = 0;
		context->bits->cr = 0;
		context->bits->smx = 0;
	}

  pthread_mutex_unlock(&context->mutex);
}

int radeontop_get_family(radeontop_context *context, unsigned int id) {
  int res = 0;

	switch(id) {
		#define CHIPSET(a,b,c) case a: res = c; break;
		#include "r600_pci_ids.h"
		#undef CHIPSET
	}

  if (res != 0) {
    init_bits(context, res);
  }

	return res;
}
