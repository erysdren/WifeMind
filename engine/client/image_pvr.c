#include "image.h"

#ifdef IMAGEFMT_PVR

typedef struct gbix {
	uint32_t magic;
	uint32_t len;
} gbix_t;

typedef struct pvr {
	uint32_t magic;
	uint32_t len_file;
	uint32_t type;
	uint16_t width;
	uint16_t height;
} pvr_t;

typedef uint32_t (*pvr_pixel_func_t)(uint16_t color);
typedef qbyte *(*pvr_image_func_t)(pvr_t *pvr, int offset, qboolean detwiddle, pvr_pixel_func_t pixel_func);

enum {
	PVR_PIXEL_TYPE_ARGB1555 = 0,
	PVR_PIXEL_TYPE_RGB565 = 1,
	PVR_PIXEL_TYPE_ARGB4444 = 2
};

enum {
	PVR_IMAGE_TYPE_TWIDDLED = 1,
	PVR_IMAGE_TYPE_TWIDDLED_MM = 2,
	PVR_IMAGE_TYPE_VQ = 3,
	PVR_IMAGE_TYPE_VQ_MM = 4,
	PVR_IMAGE_TYPE_RECTANGULAR = 9,
	PVR_IMAGE_TYPE_RECTANGULAR_MM = 10
};

static int pvr_log2(int x)
{
	switch (x)
	{
		case 8: return 3;
		case 16: return 4;
		case 32: return 5;
		case 64: return 6;
		case 128: return 7;
		case 256: return 8;
		case 512: return 9;
		case 1024: return 10;
		default: return -1;
	}
}

static int pvr_detwiddle(int x, int y, int w, int h)
{
	int wmax, hmax;
	int i, idx = 0;

	wmax = pvr_log2(w);
	hmax = pvr_log2(h);

	if (wmax < 0 || hmax < 0)
		return -1;

	for (i = 0; i < 10; i++)
	{
		if (i < wmax && i < hmax)
		{
			idx |= ((y >> i) & 1) << (i * 2 + 0);
			idx |= ((x >> i) & 1) << (i * 2 + 1);
		}
		else if (i < wmax)
		{
			idx |= ((x >> i) & 1) << (i + hmax);
		}
		else if (i < hmax)
		{
			idx |= ((y >> i) & 1) << (i + wmax);
		}
		else
		{
			break;
		}
	}

	return idx;
}

static uint32_t pvr_argb1555_to_rgba8888(uint16_t color)
{
	uint8_t r, g, b, a;
	r = ((color >> 10) & 31) << 4;
	g = ((color >> 5) & 31) << 4;
	b = ((color >> 0) & 31) << 4;
	a = ((color >> 15) & 1) * 255;
	return (a << 24) | (b << 16) | (g << 8) | r;
}

static uint32_t pvr_rgb565_to_rgba8888(uint16_t color)
{
	uint8_t r, g, b, a;
	r = ((color >> 11) & 31) << 3;
	g = ((color >> 5) & 63) << 2;
	b = ((color >> 0) & 31) << 3;
	a = 255;
	return (a << 24) | (b << 16) | (g << 8) | r;
}

static uint32_t pvr_argb4444_to_rgba8888(uint16_t color)
{
	uint8_t r, g, b, a;
	r = ((color >> 8) & 15) << 4;
	g = ((color >> 4) & 15) << 4;
	b = ((color >> 0) & 15) << 4;
	a = ((color >> 12) & 15) | 0xF;
	return (a << 24) | (b << 16) | (g << 8) | r;
}

static int pvr_mm_offset(int w)
{
	switch (w)
	{
		case 1: return 0x00006;
		case 2: return 0x00008;
		case 4: return 0x00010;
		case 8: return 0x00030;
		case 16: return 0x000B0;
		case 32: return 0x002B0;
		case 64: return 0x00AB0;
		case 128: return 0x02AB0;
		case 256: return 0x0AAB0;
		case 512: return 0x2AAB0;
		case 1024: return 0xAAAB0;
		default: return -1;
	}
}

static int pvr_mm_offset_vq(int w)
{
	switch (w)
	{
		case 1: return 0x00000;
		case 2: return 0x00001;
		case 4: return 0x00002;
		case 8: return 0x00006;
		case 16: return 0x00016;
		case 32: return 0x00056;
		case 64: return 0x00156;
		case 128: return 0x00556;
		case 256: return 0x01556;
		case 512: return 0x05556;
		case 1024: return 0x15556;
		default: return -1;
	}
}

static qbyte *pvr_decode_mm(pvr_t *pvr, pvr_image_func_t image_func, pvr_pixel_func_t pixel_func, qboolean vq, qboolean detwiddle)
{
	int offset = vq ? pvr_mm_offset_vq(pvr->width) : pvr_mm_offset(pvr->width);
	return image_func(pvr, offset, detwiddle, pixel_func);
}

static qbyte *pvr_decode(pvr_t *pvr, int offset, qboolean detwiddle, pvr_pixel_func_t pixel_func)
{
	int x, y;
	uint16_t *rgb16;
	uint32_t *rgba32;
	qbyte *ret;

	ret = (qbyte *)BZ_Malloc(pvr->width * pvr->height * (pixel_func ? sizeof(uint32_t) : sizeof(uint16_t)));
	rgb16 = (uint16_t *)ret;
	rgba32 = (uint32_t *)ret;

	for (y = 0; y < pvr->height; y++)
	{
		for (x = 0; x < pvr->width; x++)
		{
			uint16_t color;
			int ofs;
			if (detwiddle)
				ofs = offset + pvr_detwiddle(x, y, pvr->width, pvr->height) * 2;
			else
				ofs = offset + (y * pvr->width + x) * 2;

			if (ofs < 0)
			{
				BZ_Free(ret);
				return NULL;
			}

			color = *(uint16_t *)(((qbyte *)(pvr + 1)) + ofs);

			if (pixel_func)
				rgba32[y * pvr->width + x] = pixel_func(color);
			else
				rgb16[y * pvr->width + x] = color;
		}
	}

	return ret;
}

static qbyte *pvr_decode_vq(pvr_t *pvr, int offset, qboolean detwiddle, pvr_pixel_func_t pixel_func)
{
	int x, y;
	uint16_t *rgb16;
	uint32_t *rgba32;
	uint16_t *codebook;
	uint8_t *indices;
	qbyte *ret;

	ret = (qbyte *)BZ_Malloc(pvr->width * pvr->height * (pixel_func ? sizeof(uint32_t) : sizeof(uint16_t)));
	rgb16 = (uint16_t *)ret;
	rgba32 = (uint32_t *)ret;

	codebook = (uint16_t *)(pvr + 1);
	indices = ((uint8_t *)(codebook + 1024)) + offset;

	for (y = 0; y < pvr->height / 2; y++)
	{
		for (x = 0; x < pvr->width / 2; x++)
		{
			uint16_t *colors;
			int a, b, c, d;
			int idx;
			if (detwiddle)
				idx = pvr_detwiddle(x, y, pvr->width, pvr->height);
			else
				idx = y * (pvr->width / 2) + x;

			if (idx < 0)
			{
				BZ_Free(ret);
				return NULL;
			}

			colors = &codebook[indices[idx] * 4];

			a = ((y * 2) + 0) * pvr->width + ((x * 2) + 0);
			b = ((y * 2) + 1) * pvr->width + ((x * 2) + 0);
			c = ((y * 2) + 0) * pvr->width + ((x * 2) + 1);
			d = ((y * 2) + 1) * pvr->width + ((x * 2) + 1);

			if (pixel_func)
			{
				rgba32[a] = pixel_func(colors[0]);
				rgba32[b] = pixel_func(colors[1]);
				rgba32[c] = pixel_func(colors[2]);
				rgba32[d] = pixel_func(colors[3]);
			}
			else
			{
				rgb16[a] = colors[0];
				rgb16[b] = colors[1];
				rgb16[c] = colors[2];
				rgb16[d] = colors[3];
			}
		}
	}

	return ret;
}

qbyte *ReadPVRFile(qbyte *buf, int len, int *width, int *height, uploadfmt_t *format, qboolean force_rgba8)
{
	pvr_t *pvr;
	int pixel_type, image_type;
	pvr_pixel_func_t pixel_func;
	qbyte *ret = NULL;

	// skip gbix
	if (memcmp(buf, "GBIX", 4) == 0)
	{
		gbix_t *gbix = (gbix_t *)buf;
		buf += sizeof(gbix_t) + LittleLong(gbix->len);
	}

	// check magic identifier
	if (memcmp(buf, "PVRT", 4) != 0)
		return NULL;

	// fix up header
	pvr = (pvr_t *)buf;
	pvr->len_file = LittleLong(pvr->len_file);
	pvr->type = LittleLong(pvr->type);
	pvr->width = LittleShort(pvr->width);
	pvr->height = LittleShort(pvr->height);

	// break out type values
	pixel_type = pvr->type & 0xFF;
	image_type = (pvr->type & 0xFF00) >> 8;

	// get pixel function
	if (force_rgba8)
	{
		switch (pixel_type)
		{
			case PVR_PIXEL_TYPE_ARGB1555: pixel_func = pvr_argb1555_to_rgba8888; break;
			case PVR_PIXEL_TYPE_RGB565: pixel_func = pvr_rgb565_to_rgba8888; break;
			case PVR_PIXEL_TYPE_ARGB4444: pixel_func = pvr_argb4444_to_rgba8888; break;
			default: return NULL;
		}
	}
	else
	{
		pixel_func = NULL;
	}

	// decompress image
	switch (image_type)
	{
		case PVR_IMAGE_TYPE_TWIDDLED:
		{
			ret = pvr_decode(pvr, 0, true, pixel_func);
			break;
		}
		case PVR_IMAGE_TYPE_TWIDDLED_MM:
		{
			ret = pvr_decode_mm(pvr, pvr_decode, pixel_func, false, true);
			break;
		}
		case PVR_IMAGE_TYPE_VQ:
		{
			ret = pvr_decode_vq(pvr, 0, true, pixel_func);
			break;
		}
		case PVR_IMAGE_TYPE_VQ_MM:
		{
			ret = pvr_decode_mm(pvr, pvr_decode_vq, pixel_func, true, true);
			break;
		}
		case PVR_IMAGE_TYPE_RECTANGULAR:
		{
			ret = pvr_decode(pvr, 0, false, pixel_func);
			break;
		}
		case PVR_IMAGE_TYPE_RECTANGULAR_MM:
		{
			ret = pvr_decode_mm(pvr, pvr_decode, pixel_func, false, false);
			break;
		}
		default:
		{
			return NULL;
		}
	}

	// something failed
	if (!ret)
		return NULL;

	// return stuff
	if (width)
		*width = pvr->width;
	if (height)
		*height = pvr->height;
	if (format)
	{
		if (force_rgba8)
		{
			*format = PTI_RGBA8;
		}
		else
		{
			switch (pixel_type)
			{
				case PVR_PIXEL_TYPE_ARGB1555: *format = PTI_ARGB1555; break;
				case PVR_PIXEL_TYPE_RGB565: *format = PTI_RGB565; break;
				case PVR_PIXEL_TYPE_ARGB4444: *format = PTI_ARGB4444; break;
			}
		}
	}
	return ret;
}

#endif // IMAGEFMT_PVR
