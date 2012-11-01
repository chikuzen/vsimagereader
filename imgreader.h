/*
  imgreader.h

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


#ifndef VS_IMG_READ_H
#define VS_IMG_READ_H

#include <stdint.h>
#include "VapourSynth.h"

#define IMG_ORDER_BGR 0x0100
#define IMG_ORDER_RGB 0x0200

typedef struct img_handler img_hnd_t;
typedef int (VS_CC *func_read_image)(img_hnd_t *, int);
typedef void (VS_CC *func_write_frame)(img_hnd_t *, VSFrameRef *, const VSAPI *);

typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t reserved;
} color_palette_t;

struct img_handler {
    const char **src_files;
    unsigned long *src_size;
    uint8_t *src_buff;
    uint8_t *img_buff;
    uint8_t **img_buff_index;
    void *tjhandle;
    func_read_image *read_image;
    func_write_frame write_frame;
    VSVideoInfo vi;
    color_palette_t palettes[256];
    int row_adjust;
    uint32_t misc;
};


void VS_CC write_planar(img_hnd_t *ih, VSFrameRef *dst, const VSAPI *vsapi);

void VS_CC write_rgb24(img_hnd_t *ih, VSFrameRef *dst, const VSAPI *vsapi);

void VS_CC write_rgb32(img_hnd_t *ih, VSFrameRef *dst, const VSAPI *vsapi);

void VS_CC write_rgb48(img_hnd_t *ih, VSFrameRef *dst, const VSAPI *vsapi);

void VS_CC write_palette(img_hnd_t *ih, VSFrameRef *dst, const VSAPI *vsapi);

#endif
