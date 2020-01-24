/*
    Copyright (C) 2020 Hannes Schulze

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
#include "version.h"
#include <pthread.h>

radeontop_context *radeontop_context_init(radeontop_die_func on_die) {
  radeontop_context *res = malloc(sizeof(radeontop_context));

  res->die_func = on_die;
  res->sclk_max = 0;
  res->mclk_max = 0;
  res->getgrbm = NULL;
  res->getvram = NULL;
  res->getgtt = NULL;
  res->getsclk = NULL;
  res->getmclk = NULL;
  res->results = NULL;

  pthread_mutex_init(&res->mutex, NULL);

  return res;
}

const char *radeontop_version() {
  return VERSION;
}

radeontop_bits *radeontop_context_get_results(radeontop_context *context) {
  radeontop_bits *res = NULL;

  pthread_mutex_lock(&context->mutex);
  res = context->results;
  pthread_mutex_unlock(&context->mutex);

  return res;
}
