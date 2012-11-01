/*
  pngread.c:

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


#include "pngread.h"
#include "png.h"

#define PNG_SIG_LENGTH 8
#define PNG_TRANSFORM_SETTINGS \
    (PNG_TRANSFORM_STRIP_ALPHA | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_SWAP_ENDIAN)


#define COLOR_OR_BITS(color, bits) \
    (((uint32_t)color << 16) | (uint32_t)bits)
static VSPresetFormat VS_CC get_dst_format(int color_type, int bits)
{
    uint32_t p_color = COLOR_OR_BITS(color_type, bits);
    const struct {
        uint32_t png_color_type;
        VSPresetFormat vsformat;
    } table[] = {
        { COLOR_OR_BITS(PNG_COLOR_TYPE_GRAY,  8),  pfGray8  },
        { COLOR_OR_BITS(PNG_COLOR_TYPE_GRAY, 16),  pfGray16 },
        { COLOR_OR_BITS(PNG_COLOR_TYPE_RGB,   8),  pfRGB24  },
        { COLOR_OR_BITS(PNG_COLOR_TYPE_RGB,  16),  pfRGB48  },
        { p_color, pfNone }
    };

    int i = 0;
    while (table[i].png_color_type != p_color) i++;
    return table[i].vsformat;
}
#undef COLOR_OR_BITS


const char * VS_CC
check_png(FILE *fp, int *width, int *height, VSPresetFormat *format,
          uint32_t *max_image_size)
{
    uint8_t signature[PNG_SIG_LENGTH];
    if (fread(signature, 1, PNG_SIG_LENGTH, fp) != PNG_SIG_LENGTH ||
        png_sig_cmp(signature, 0, PNG_SIG_LENGTH)) {
        return "unsupported format";
    }

    png_structp p_str =
        png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!p_str) {
        return "failed to create png_read_struct";
    }

    png_infop p_info = png_create_info_struct(p_str);
    if (!p_info) {
        png_destroy_read_struct(&p_str, NULL, NULL);
        return "failed to create png_info_struct";
    }

    png_init_io(p_str, fp);
    png_set_sig_bytes(p_str, PNG_SIG_LENGTH);
    png_read_info(p_str, p_info);

    png_uint_32 w, h;
    int color_type, bit_depth;
    png_get_IHDR(p_str, p_info, &w, &h, &bit_depth, &color_type,
                 NULL, NULL, NULL);
    if (color_type & PNG_COLOR_MASK_ALPHA) {
        png_set_strip_alpha(p_str);
    }
    if (color_type & PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(p_str);
    }
    if (bit_depth < 8) {
        png_set_packing(p_str);
    }

    png_read_update_info(p_str, p_info);
    png_get_IHDR(p_str, p_info, &w, &h, &bit_depth, &color_type,
                 NULL, NULL, NULL);
    uint32_t image_size = png_get_rowbytes(p_str, p_info) * h;

    png_destroy_read_struct(&p_str, &p_info, NULL);

    *width = w;
    *height = h;
    *format = get_dst_format(color_type, bit_depth);
    if (*format == pfNone) {
        return "unsupported png color type";
    }

    if (image_size > *max_image_size) {
        *max_image_size = image_size;
    }

    return NULL;
}


int VS_CC read_png(img_hnd_t *ih, int n)
{
    FILE *fp = fopen(ih->src_files[n], "rb");
    if (!fp) {
        return -1;
    }

    png_structp p_str =
        png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!p_str) {
        fclose(fp);
        return -1;
    }

    png_infop p_info = png_create_info_struct(p_str);
    if (!p_info) {
        fclose(fp);
        png_destroy_read_struct(&p_str, NULL, NULL);
        return -1;
    }

    png_init_io(p_str, fp);
    png_set_rows(p_str, p_info, ih->img_buff_index);
    png_read_png(p_str, p_info, PNG_TRANSFORM_SETTINGS, NULL);

    fclose(fp);
    png_destroy_read_struct(&p_str, &p_info, NULL);

    ih->misc = IMG_ORDER_RGB;
    ih->row_adjust = 1;
    switch (ih->vi.format->id) {
    case pfRGB24:
        ih->write_frame = write_rgb24;
        break;
    case pfRGB48:
        ih->write_frame = write_rgb48;
        break;
    default:
        ih->write_frame = write_planar;
        break;
    }

    return 0;
}
