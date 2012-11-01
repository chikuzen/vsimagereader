/*
  jpgread.c:

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


#include <stdlib.h>
#include <sys/stat.h>
#include "imgreader.h"
#include "jpgread.h"


static VSPresetFormat VS_CC tjsamp_to_vspresetformat(enum TJSAMP tjsamp)
{
    const struct {
        enum TJSAMP tjsample_type;
        VSPresetFormat vsformat;
    } table[] = {
        { TJSAMP_444,  pfYUV444P8 },
        { TJSAMP_422,  pfYUV422P8 },
        { TJSAMP_420,  pfYUV420P8 },
        { TJSAMP_GRAY, pfGray8    },
        { TJSAMP_440,  pfYUV440P8 },
        { tjsamp,      pfNone     }
    };

    int i = 0;
    while (table[i].tjsample_type != tjsamp) i++;
    return table[i].vsformat;
}


const char * VS_CC
check_jpg(img_hnd_t *ih, int n, uint32_t *max_src_size, FILE *fp,
          int *width, int *height, VSPresetFormat *format,
          uint32_t *max_image_size)
{
    struct stat st;
    if (stat(ih->src_files[n], &st)) {
        return "source file does not exist";
    }
    ih->src_size[n] = st.st_size;
    if (*max_src_size < st.st_size) {
        *max_src_size = st.st_size;
        free(ih->src_buff);
        ih->src_buff = malloc(*max_src_size);
        if (!ih->src_buff) {
            return "failed to allocate read buffer";
        }
    }

    unsigned long read = fread(ih->src_buff, 1, st.st_size, fp);
    fclose(fp);
    if (read < st.st_size) {
        return "failed to read jpeg file";
    }

    int subsample;
    tjhandle handle = (tjhandle)ih->tjhandle;
    if (tjDecompressHeader2(
            handle, ih->src_buff, read, width, height, &subsample) != 0) {
        return tjGetErrorStr();
    }

    if (subsample == TJSAMP_420 || subsample == TJSAMP_422) {
        *width += *width & 1;
    }
    if (subsample == TJSAMP_420 || subsample == TJSAMP_440) {
        *height += *height & 1;
    }
    *format = tjsamp_to_vspresetformat(subsample);

    uint32_t image_size = tjBufSizeYUV(*width, *height, subsample);
    if (image_size > *max_image_size) {
        *max_image_size = image_size;
    }

    return NULL;
}


int VS_CC read_jpg(img_hnd_t *ih, int n)
{
    FILE *fp = fopen(ih->src_files[n], "rb");
    if (!fp) {
        return -1;
    }

    unsigned long read = fread(ih->src_buff, 1, ih->src_size[n], fp);
    fclose(fp);
    if (read < ih->src_size[n]) {
        return -1;
    }

    tjhandle tjh = (tjhandle)ih->tjhandle;
    if (tjDecompressToYUV(tjh, ih->src_buff, read, ih->img_buff, 0)) {
        return -1;
    }

    ih->write_frame = write_planar;
    ih->row_adjust = 4;

    return 0;
}
