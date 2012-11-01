/*
  writeframe.c

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


#include <string.h>
#include "imgreader.h"

static const int rgb[3] = {0, 1, 2};
static const int bgr[3] = {2, 1, 0};


static inline uint32_t VS_CC
bitor8to32(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3)
{
    return ((uint32_t)b0 << 24) | ((uint32_t)b1 << 16) |
           ((uint32_t)b2 << 8) | (uint32_t)b3;
}


static void VS_CC
bit_blt(VSFrameRef *dst, int plane, const VSAPI *vsapi, uint8_t *srcp,
        int row_size, int height)
{
    uint8_t *dstp = vsapi->getWritePtr(dst, plane);
    int dst_stride = vsapi->getStride(dst, plane);

    if (row_size == dst_stride) {
        memcpy(dstp, srcp, row_size * height);
        return;
    }

    for (int i = 0; i < height; i++) {
        memcpy(dstp, srcp, row_size);
        dstp += dst_stride;
        srcp += row_size;
    }
}


void VS_CC write_planar(img_hnd_t *ih, VSFrameRef *dst, const VSAPI *vsapi)
{
    uint8_t *srcp = ih->img_buff;

    for (int i = 0, num = ih->vi.format->numPlanes; i < num; i++) {
        int row_size
            = vsapi->getFrameWidth(dst, i) * ih->vi.format->bytesPerSample;
        row_size = (row_size + ih->row_adjust) & (~ih->row_adjust);
        int height = vsapi->getFrameHeight(dst, i);
        bit_blt(dst, i, vsapi, srcp, row_size, height);
        srcp += row_size * height;
    }
}


void VS_CC write_rgb24(img_hnd_t *ih, VSFrameRef *dst, const VSAPI *vsapi)
{
    typedef struct {
        uint8_t c[12];
    } rgb24_t;

    uint8_t *srcp_orig = ih->img_buff;
    int row_size = (ih->vi.width + 3) >> 2;
    int height = ih->vi.height;
    int src_stride = (ih->vi.width * 3 + ih->row_adjust) & (~ih->row_adjust);

    const int *order = (ih->misc & IMG_ORDER_RGB) ? rgb : bgr;
    uint32_t *dstp0 = (uint32_t *)vsapi->getWritePtr(dst, order[0]);
    uint32_t *dstp1 = (uint32_t *)vsapi->getWritePtr(dst, order[1]);
    uint32_t *dstp2 = (uint32_t *)vsapi->getWritePtr(dst, order[2]);
    int dst_stride = vsapi->getStride(dst, 0) >> 2;

    for (int y = 0; y < height; y++) {
        rgb24_t *srcp = (rgb24_t *)(srcp_orig + y * src_stride);
        for (int x = 0; x < row_size; x++) {
            dstp0[x] = bitor8to32(srcp[x].c[9], srcp[x].c[6],
                                  srcp[x].c[3], srcp[x].c[0]);
            dstp1[x] = bitor8to32(srcp[x].c[10], srcp[x].c[7],
                                  srcp[x].c[4], srcp[x].c[1]);
            dstp2[x] = bitor8to32(srcp[x].c[11], srcp[x].c[8],
                                  srcp[x].c[5], srcp[x].c[2]);
        }
        dstp0 += dst_stride;
        dstp1 += dst_stride;
        dstp2 += dst_stride;
    }
}


void VS_CC
write_rgb32(img_hnd_t *ih, VSFrameRef *dst, const VSAPI *vsapi)
{
    typedef struct {
        uint8_t c[16];
    } rgb32_t;

    uint8_t *srcp_orig = ih->img_buff;
    int row_size = (ih->vi.width + 3) >> 2;
    int height = ih->vi.height;
    int src_stride = (ih->vi.width * 4 + ih->row_adjust) & (~ih->row_adjust);

    const int *order = (ih->misc & IMG_ORDER_RGB) ? rgb : bgr;
    uint32_t *dstp0 = (uint32_t *)vsapi->getWritePtr(dst, order[0]);
    uint32_t *dstp1 = (uint32_t *)vsapi->getWritePtr(dst, order[1]);
    uint32_t *dstp2 = (uint32_t *)vsapi->getWritePtr(dst, order[2]);
    int dst_stride = vsapi->getStride(dst, 0) >> 2;

    for (int y = 0; y < height; y++) {
        rgb32_t *srcp = (rgb32_t *)(srcp_orig + y * src_stride);
        for (int x = 0; x < row_size; x++) {
            dstp0[x] = bitor8to32(srcp[x].c[12], srcp[x].c[8],
                                  srcp[x].c[4],  srcp[x].c[0]);
            dstp1[x] = bitor8to32(srcp[x].c[13], srcp[x].c[9],
                                  srcp[x].c[5],  srcp[x].c[1]);
            dstp2[x] = bitor8to32(srcp[x].c[14], srcp[x].c[10],
                                  srcp[x].c[6],  srcp[x].c[2]);
        }
        dstp0 += dst_stride;
        dstp1 += dst_stride;
        dstp2 += dst_stride;
    }
}


void VS_CC
write_rgb48(img_hnd_t *ih, VSFrameRef *dst, const VSAPI *vsapi)
{
    typedef struct {
        uint16_t c[3];
    } rgb48_t;

    uint8_t *srcp_orig = ih->img_buff;
    int row_size = ih->vi.width;
    int height = ih->vi.height;
    int src_stride = (ih->vi.width * 6 + ih->row_adjust) & (~ih->row_adjust);

    const int *order = (ih->misc & IMG_ORDER_RGB) ? rgb : bgr;
    uint16_t *dstp0 = (uint16_t *)vsapi->getWritePtr(dst, order[0]);
    uint16_t *dstp1 = (uint16_t *)vsapi->getWritePtr(dst, order[1]);
    uint16_t *dstp2 = (uint16_t *)vsapi->getWritePtr(dst, order[2]);
    int stride = vsapi->getStride(dst, 0) >> 1;;

    for (int y = 0; y < height; y++) {
        rgb48_t *srcp = (rgb48_t *)(srcp_orig + y * src_stride);
        for (int x = 0; x < row_size; x++) {
            dstp0[x] = srcp[x].c[0];
            dstp1[x] = srcp[x].c[1];
            dstp2[x] = srcp[x].c[2];
        }
        dstp0 += stride;
        dstp1 += stride;
        dstp2 += stride;
    }
}


void VS_CC
write_palette(img_hnd_t *ih, VSFrameRef *dst, const VSAPI *vsapi)
{
    color_palette_t *palette = ih->palettes;
    int bits_per_pix = ih->misc & 0xFF;

    uint8_t *srcp_orig = ih->img_buff;
    int row_size = ih->vi.width;
    int height = ih->vi.height;
    int src_stride = ((ih->vi.width * bits_per_pix + 7) / 8 + ih->row_adjust)
                     & (~ih->row_adjust);

    uint8_t *dstp_b = vsapi->getWritePtr(dst, 2);
    uint8_t *dstp_g = vsapi->getWritePtr(dst, 1);
    uint8_t *dstp_r = vsapi->getWritePtr(dst, 0);
    int dst_stride = vsapi->getStride(dst, 0);

    uint8_t mask = (1 << bits_per_pix) - 1;

    for (int y = 0; y < height; y++) {
        uint8_t *srcp = srcp_orig + y * src_stride;
        for (int x = 0, shift = 8; x < row_size; x++) {
            shift -= bits_per_pix;
            dstp_b[x] = palette[(*srcp >> shift) & mask].blue;
            dstp_g[x] = palette[(*srcp >> shift) & mask].green;
            dstp_r[x] = palette[(*srcp >> shift) & mask].red;
            if (shift < 1) {
                shift = 8;
                srcp++;
            }
        }
        dstp_b += dst_stride;
        dstp_g += dst_stride;
        dstp_r += dst_stride;
    }
}
