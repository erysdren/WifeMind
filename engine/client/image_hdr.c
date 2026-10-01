#include "image.h"

#ifdef IMAGEFMT_HDR			//baselayer only.

//Radiance files are some weird BGRE8 hdr format that somehow managed to get reasonably well supported
void *ReadRadianceFile(qbyte *buf, size_t len, const char *fname, int *width, int *height, uploadfmt_t *format)
{	//this isn't expected to be fast.
	qbyte *end = buf+len;
	size_t l, x, y, w, h, e;
	float *r, *o, m;
	qbyte rgbe[4];

	char fmt[128];
	char line[256];
	byte_vec4_t *row = NULL;
	w = h = 0;
	*fmt = 0;
	while (buf < end)
	{
		l = 0;
		while(buf < end && *buf != '\n')
		{
			if (*buf == '\r' && buf[1] == '\n')
				continue;
			if (l < countof(line)-1)
				line[l++] = *buf++;
		}
		line[l] = 0;
		buf++;
		if (!strncmp(line, "FORMAT=", 7))
			Q_strncpyz(fmt, line+7, sizeof(fmt));
		if (!l)
			break;
	}
	if (strncmp(buf, "-Y ", 3))
	{
		Con_Printf("%s uses unsupported orientation\n", fname);
		return NULL;
	}
	h = strtol(buf+3, (char**)&buf, 0);
	if (strncmp(buf, " +X ", 4))
	{
		Con_Printf("%s uses unsupported orientation\n", fname);
		return NULL;
	}
	w = strtol(buf+4, (char**)&buf, 0);
	if (*buf == '\r')
		buf++;
	if (*buf++ != '\n')
		return NULL;

	if (strcmp(fmt, "32-bit_rle_rgbe"))
	{
		Con_Printf("%s uses unsupported pixel format (%s)\n", fname, fmt);
		return NULL;
	}

	r = o = BZ_Malloc(sizeof(float)*4*w*h);
	if (!r)
		return NULL;
	for (y=0; y < h && buf+4<end; y++)
	{
		if (buf[0] == 2 && buf[1] == 2 && buf[2] < 127)
		{ //'new' rle logic
			int c, v;
			buf+=4;
			if (!row)
				row = BZ_Malloc(w*sizeof(*row));
			//encoded separately
			for (c = 0; c < 4; c++)
			{
				for (x=0; x < w; )
				{
					if (buf+1 >= end)
						goto fail;
					if (*buf > 128)
					{
						e = x + (*buf++ - 128);
						v = *buf++;
						if (e == x || e > w)
							goto fail;
						while (x < e)
							row[x++][c] = v;
					}
					else
					{
						e = x + *buf++;
						if (e == x || e > w || buf+(e-x) >= end)
							goto fail;
						while (x < e)
							row[x++][c] = *buf++;
					}
				}
			}

			//decompressed the entire line, can make sense of it now.
			for (x=0; x < w; x++)
			{
				m = ldexp(1,row[x][3]-136);
				*o++ = m * row[x][0];
				*o++ = m * row[x][1];
				*o++ = m * row[x][2];
				*o++ = 1;
			}
		}
		else if (buf[0] == 1 && buf[1] == 1 && buf[2] == 1)
		{ //old rle logic
			Con_Printf("%s uses unsupported (old) RLE compression\n", fname);
			goto fail;
		}
		else
		{
			for (x=0; x < w && buf+4<end ; x++)
			{
				rgbe[0] = *buf++;
				rgbe[1] = *buf++;
				rgbe[2] = *buf++;
				rgbe[3] = *buf++;

				m = ldexp(1,rgbe[3]-136);
				*o++ = m * rgbe[0];
				*o++ = m * rgbe[1];
				*o++ = m * rgbe[2];
				*o++ = 1;
			}
		}
	}
	if (row)
		BZ_Free(row);
	*width = w;
	*height = h;
	//FIXME: should probably convert to e5bgr9 or something.
	*format = PTI_RGBA32F;
	return r;
fail:
	if (row)
		BZ_Free(row);
	BZ_Free(r);
	return NULL;
}

#endif // IMAGEFMT_HDR
