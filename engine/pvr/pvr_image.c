#include "quakedef.h"
#ifdef PVRQUAKE
#include "pvr.h"

void PVR_DestroyTexture(texid_t tex)
{
	pvrimage_t *img = tex->ptr;
	tex->ptr = NULL;

	/*make sure its not in use by the renderer*/
	SWRast_Sync(&commandqueue);

	/*okay, it can be killed*/
	pvr_mem_free(img->data);
	BZ_Free(img);
}

qboolean PVR_LoadTextureMips(texid_t tex, const struct pendingtextureinfo *mips)
{
	pvrimage_t *img;

	if (mips->type != PTI_2D)
		return false;

	if (mips->mip[0].datasize % 32)
		return false;

	//only accept formats that actually make sense here.
	switch(mips->encoding)
	{
	case PTI_ARGB1555: //16bit alpha format
	case PTI_ARGB1555_TW: //16bit alpha format, twiddled
	case PTI_ARGB1555_TW_VQ: //16bit alpha format, vq compressed, twiddled
	case PTI_RGB565: //16bit format
	case PTI_RGB565_TW: //16bit format, twiddled
	case PTI_RGB565_TW_VQ: //16bit format, vq compressed, twiddled
	case PTI_ARGB4444: //16bit alpha format
	case PTI_ARGB4444_TW: //16bit alpha format, twiddled
	case PTI_ARGB4444_TW_VQ: //16bit alpha format, vq compressed, twiddled
		break;
	default:
		return false;
	}

	img = BZ_Malloc(sizeof(pvrimage_t));
	tex->ptr = img;

	img->encoding = mips->encoding;
	img->width = mips->mip[0].width;
	img->height = mips->mip[0].height;
	img->size = mips->mip[0].datasize;
	img->data = pvr_mem_malloc(img->size);

	if (!img->data) {
		Con_Printf("Failed to allocate %d of texture memory\n", img->size);
		BZ_Free(img);
		return false;
	}

	pvr_txr_load(mips->mip[0].data, img->data, img->size);

	return true;
}

#endif
