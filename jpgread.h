/*
  jpgread.h

  This file is part of vsimagereader

  Copyright (C) 2012  Oka Motofumi

  Author: Oka Motofumi (chikuzen.mo at gmail dot com)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with Libav; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/


#ifndef VS_IMG_READ_JPG_H
#define VS_IMG_READ_JPG_H

#include <stdio.h>
#include <stdint.h>
#include "turbojpeg.h"
#include "imgreader.h"
#include "VapourSynth.h"

const char * VS_CC
check_jpg(img_hnd_t *ih, int n, uint32_t *max_src_size, FILE *fp,
          int *width, int *height, VSPresetFormat *format,
          uint32_t *max_image_size);

int VS_CC read_jpg(img_hnd_t *ih, int n);

#endif