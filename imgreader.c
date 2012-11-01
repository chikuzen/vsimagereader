/*
  imgreader.c: Image reader for VapourSynth

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
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "imgreader.h"
#include "bmpread.h"
#include "jpgread.h"
#include "pngread.h"
#include "tgaread.h"
#include "VapourSynth.h"

#define VS_IMGR_VERSION "0.1.0"
#define INITIAL_SRC_BUFF_SIZE (2 * 1024 * 1024) /* 2MiByte */

typedef enum {
    IMG_TYPE_NONE,
    IMG_TYPE_BMP,
    IMG_TYPE_JPG,
    IMG_TYPE_PNG,
    IMG_TYPE_TGA
} image_type_t;


typedef struct {
    const VSMap *in;
    VSMap *out;
    VSCore *core;
    const VSAPI *vsapi;
} vs_args_t;


static const VSFrameRef * VS_CC
img_get_frame(int n, int activation_reason, void **instance_data,
              void **frame_data, VSFrameContext *frame_ctx, VSCore *core,
              const VSAPI *vsapi)
{
    if (activation_reason != arInitial) {
        return NULL;
    }
    
    img_hnd_t *ih = (img_hnd_t *)*instance_data;

    int frame_number = n;
    if (n >= ih->vi.numFrames) {
        frame_number = ih->vi.numFrames - 1;
    }

    if (ih->read_image[frame_number](ih, frame_number)) {
        return NULL;
    }
    ih->row_adjust--;

    VSFrameRef *dst = vsapi->newVideoFrame(ih->vi.format, ih->vi.width,
                                           ih->vi.height, NULL, core);

    VSMap *props = vsapi->getFramePropsRW(dst);
    vsapi->propSetInt(props, "_DurationNum", ih->vi.fpsDen, 0);
    vsapi->propSetInt(props, "_DurationDen", ih->vi.fpsNum, 0);

    ih->write_frame(ih, dst, vsapi);

    return dst;
}

static void close_handler(img_hnd_t *ih)
{
    if (!ih) {
        return;
    }
    if (ih->tjhandle && tjDestroy((tjhandle)ih->tjhandle)) {
        fprintf(stderr, tjGetErrorStr());
    }
    free(ih->read_image);
    free(ih->img_buff_index);
    free(ih->img_buff);
    free(ih->src_buff);
    free(ih->src_size);
    free(ih->src_files);
    free(ih);
}


static void VS_CC
vs_init(VSMap *in, VSMap *out, void **instance_data, VSNode *node,
        VSCore *core, const VSAPI *vsapi)
{
    img_hnd_t *ih = (img_hnd_t *)*instance_data;
    vsapi->setVideoInfo(&ih->vi, node);
}


static void VS_CC
vs_close(void *instance_data, VSCore *core, const VSAPI *vsapi)
{
    img_hnd_t *ih = (img_hnd_t *)instance_data;
    close_handler(ih);
}


static image_type_t VS_CC detect_image_type(FILE *fp)
{
    uint16_t sof;
    if (fread(&sof, 1, 2, fp) != 2) {
        return IMG_TYPE_NONE;
    }

    if (sof == 0x4D42) {
        return IMG_TYPE_BMP;
    }
    if (sof == 0xD8FF) {
        return IMG_TYPE_JPG;
    }
    if (sof == 0x5089) {
        return IMG_TYPE_PNG;
    }
    if (((uint8_t *)&sof)[1] == 0x00) { // 0x01(color map) is unsupported.
        return IMG_TYPE_TGA;
    }
    
    return IMG_TYPE_NONE;
}


static const char * VS_CC
check_srcs(img_hnd_t *ih, int n, uint32_t *max_src_size,
           uint32_t *max_image_size, vs_args_t *va)
{
    FILE *fp = fopen(ih->src_files[n], "rb");
    if (!fp) {
        return "failed to open file";
    }

    image_type_t img_type = detect_image_type(fp);
    if (img_type == IMG_TYPE_NONE) {
        fclose(fp);
        return "unsupported format";
    }

    fseek(fp, 0, SEEK_SET);
    const char *ret = NULL;
    int width, height;
    VSPresetFormat format = pfRGB24;

    switch (img_type) {
    case IMG_TYPE_BMP:
        ret = check_bmp(ih, n, fp, &width, &height, max_image_size);
        ih->read_image[n] = read_bmp;
        break;
    case IMG_TYPE_JPG:
        ret = check_jpg(ih, n, max_src_size, fp, &width, &height, &format,
                        max_image_size);
        ih->read_image[n] = read_jpg;
        break;
    case IMG_TYPE_PNG:
        ret = check_png(fp, &width, &height, &format, max_image_size);
        ih->read_image[n] = read_png;
        break;
    case IMG_TYPE_TGA:
        ret = check_tga(fp, &width, &height, max_image_size);
        ih->read_image[n] = read_tga;
        break;
    default:
        break;
    }

    fclose(fp);
    if (ret) {
        return ret;
    }

    if (ih->vi.width != width || ih->vi.height != height) {
        if (n > 0) {
            return "found a file which has diffrent resolution from first file";
        }
        ih->vi.width = width;
        ih->vi.height = height;
    }

    if (!ih->vi.format) {
        ih->vi.format = va->vsapi->getFormatPreset(format, va->core);
    }

    if (ih->vi.format->id != format) {
        return "found a file witch has different destination format from first file";
    }

    return NULL;
}


static void VS_CC
set_args_int64(int64_t *p, int default_value, const char *arg, vs_args_t *va)
{
    int err;
    *p = va->vsapi->propGetInt(va->in, arg, 0, &err);
    if (err) {
        *p = default_value;
    }
}


#define RET_IF_ERR(cond, ...) \
{\
    if (cond) {\
        close_handler(ih);\
        snprintf(msg, 240, __VA_ARGS__);\
        vsapi->setError(out, msg_buff);\
        return;\
    }\
}

static void VS_CC
create_reader(const VSMap *in, VSMap *out, void *user_data, VSCore *core,
              const VSAPI *vsapi)
{
    char msg_buff[256] = "imgr: ";
    char *msg = msg_buff + strlen(msg_buff);
    vs_args_t va = {in, out, core, vsapi};

    img_hnd_t *ih = (img_hnd_t *)calloc(sizeof(img_hnd_t), 1);
    RET_IF_ERR(!ih, "failed to create handler");

    int num_srcs = vsapi->propNumElements(in, "files");
    RET_IF_ERR(num_srcs < 1, "no source file");
    ih->vi.numFrames = num_srcs;

    ih->src_files = (const char **)malloc(sizeof(char *) * num_srcs);
    RET_IF_ERR(!ih->src_files, "failed to allocate array of src files");

    ih->read_image
        = (func_read_image *)malloc(sizeof(func_read_image) * num_srcs);
    RET_IF_ERR(!ih->read_image,
               "failed to allocate array of reading image functions");

    ih->src_size = (unsigned long *)malloc(sizeof(unsigned long) * num_srcs);
    RET_IF_ERR(!ih->src_size, "failed to allocate array of src file size");

    ih->tjhandle = tjInitDecompress();
    RET_IF_ERR(!ih->tjhandle, "%s", tjGetErrorStr());

    uint32_t max_image_size = 0;
    uint32_t max_src_size = INITIAL_SRC_BUFF_SIZE;

    ih->src_buff = (uint8_t *)malloc(INITIAL_SRC_BUFF_SIZE);
    RET_IF_ERR(!ih->src_buff, "failed to allocate src read buffer");

    int err;
    for (int i = 0; i < num_srcs; i++) {
        ih->src_files[i] = vsapi->propGetData(in, "files", i, &err);
        RET_IF_ERR(err || strlen(ih->src_files[i]) == 0,
                   "zero length file name was found");
        const char *cs
            = check_srcs(ih, i, &max_src_size, &max_image_size, &va);
        RET_IF_ERR(cs, "file %d: %s", i, cs);
    }

    uint8_t *buff = (uint8_t *)malloc(max_image_size + 32);
    RET_IF_ERR(!buff, "failed to allocate image buffer");
    ih->img_buff = buff;    
    ih->img_buff_index = (uint8_t **)malloc(sizeof(uint8_t *) * ih->vi.height);
    RET_IF_ERR(!ih->img_buff_index, "failed to allocate image buffer index");
    uint32_t png_row_size = ih->vi.width * ih->vi.format->bytesPerSample
                            * ih->vi.format->numPlanes;
    for (int i = 0; i < ih->vi.height; i++) {
        ih->img_buff_index[i] = buff;
        buff += png_row_size;
    }

    set_args_int64(&ih->vi.fpsNum, 24, "fpsnum", &va);
    set_args_int64(&ih->vi.fpsDen, 1, "fpsden", &va);

    const VSNodeRef *node =
        vsapi->createFilter(in, out, "Read", vs_init, img_get_frame, vs_close,
                            fmSerial, 0, ih, core);

    vsapi->propSetNode(out, "clip", node, 0);
}


VS_EXTERNAL_API(void) VapourSynthPluginInit(
    VSConfigPlugin f_config, VSRegisterFunction f_register, VSPlugin *plugin)
{
    f_config("chikuzen.does.not.have.his.own.domain.imgr", "imgr",
             "Image reader for VapourSynth " VS_IMGR_VERSION,
             VAPOURSYNTH_API_VERSION, 1, plugin);
    f_register("Read", "files:data[];fpsnum:int:opt;fpsden:int:opt",
               create_reader, NULL, plugin);
}
