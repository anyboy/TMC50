/*
 * Copyright (c) 2013-2014 Actions Semi Co., Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <dsp.h>
#include <misc/util.h>
#include <logging/sys_log.h>
#include <mem_manager.h>
#ifdef CONFIG_DSP_LIB_IN_SDFS
#include <sdfs.h>
#else
#include "dsp_image_builtin.h"
#endif

#ifdef CONFIG_DSP_LIB_IN_SDFS
const struct dsp_imageinfo *dsp_create_image(const char *name)
{
	struct dsp_imageinfo *image = mem_malloc(sizeof(*image));
	if (image == NULL)
		return NULL;

	if (sd_fmap(name, (void **)&image->ptr, (int *)&image->size)) {
		SYS_LOG_ERR("cannot find dsp image \"%s\"", name);
		mem_free(image);
		return NULL;
	}

	strncpy(image->name, name, sizeof(image->name));
	return image;
}

void dsp_free_image(const struct dsp_imageinfo *image)
{
	mem_free((void*)image);
}

#else  /* CONFIG_DSP_LIB_IN_SDFS */

enum {
	IMG_MP3 = 0,
	IMG_AAC,
	IMG_SBC,
	IMG_AUDIOPP,
	IMG_HFP,
	IMG_SV_AL,
};

static struct dsp_imageinfo dsp_builtin_images[] = {
	[IMG_MP3] = {"admp3.dsp", NULL, 0},
	[IMG_AAC] = {"adAAC.dsp", NULL, 0},
	[IMG_SBC] = {"adSBC.dsp", NULL, 0},
	[IMG_AUDIOPP] = {"audiopp.dsp", NULL, 0},
	[IMG_HFP] = {"hfp.dsp", NULL, 0},
	[IMG_SV_AL] = {"sv_al.dsp", NULL, 0},
};

const struct dsp_imageinfo *dsp_create_image(const char *name)
{
	if (!strcmp(name, "admp3.dsp")) {
		dsp_builtin_images[IMG_MP3].ptr = DSP_IMAGE_BEGIN(admp3);
		dsp_builtin_images[IMG_MP3].size = DSP_IMAGE_SIZE(admp3);
		return &dsp_builtin_images[IMG_MP3];
	} else if (!strcmp(name, "adAAC.dsp")) {
		dsp_builtin_images[IMG_AAC].ptr = DSP_IMAGE_BEGIN(adAAC);
		dsp_builtin_images[IMG_AAC].size = DSP_IMAGE_SIZE(adAAC);
		return &dsp_builtin_images[IMG_AAC];
	} else if (!strcmp(name, "adSBC.dsp")) {
		dsp_builtin_images[IMG_SBC].ptr = DSP_IMAGE_BEGIN(adSBC);
		dsp_builtin_images[IMG_SBC].size = DSP_IMAGE_SIZE(adSBC);
		return &dsp_builtin_images[IMG_SBC];
	} else if (!strcmp(name, "audiopp.dsp")) {
		dsp_builtin_images[IMG_AUDIOPP].ptr = DSP_IMAGE_BEGIN(audiopp);
		dsp_builtin_images[IMG_AUDIOPP].size = DSP_IMAGE_SIZE(audiopp);
		return &dsp_builtin_images[IMG_AUDIOPP];
	} else if (!strcmp(name, "hfp.dsp")) {
		dsp_builtin_images[IMG_HFP].ptr = DSP_IMAGE_BEGIN(hfp);
		dsp_builtin_images[IMG_HFP].size = DSP_IMAGE_SIZE(hfp);
		return &dsp_builtin_images[IMG_HFP];
	} else if (!strcmp(name, "sv_al.dsp")) {
		dsp_builtin_images[IMG_SV_AL].ptr = DSP_IMAGE_BEGIN(sv_al);
		dsp_builtin_images[IMG_SV_AL].size = DSP_IMAGE_SIZE(sv_al);
		return &dsp_builtin_images[IMG_SV_AL];
	} else {
		SYS_LOG_ERR("cannot find dsp image \"%s\"", name);
		return NULL;
	}
}

void dsp_free_image(const struct dsp_imageinfo *image)
{
	ARG_UNUSED(image);
}
#endif /* CONFIG_DSP_LIB_IN_SDFS */

