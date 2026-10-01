#include "image.h"

#ifdef IMAGEFMT_PCX

cvar_t r_dodgypcxfiles = CVARD("r_dodgypcxfiles", "0", "When enabled, this will ignore the palette stored within pcx files, for compatibility with quake2.");

/*
==============
WritePCXfile
==============
*/
qboolean WritePCXfile (const char *filename, enum fs_relative fsroot, qbyte *data, int width, int height,
	int rowbytes, qbyte *palette, qboolean upload) //data is 8bit.
{
	int		i, j, length;
	pcx_t	*pcx;
	qbyte		*pack;
	qboolean ret;

	pcx = BZ_Malloc(width*height*2+1000);
	if (pcx == NULL)
	{
		Con_Printf("WritePCXfile: not enough memory\n");
		return false;
	}

	pcx->manufacturer = 0x0a;	// PCX id
	pcx->version = 5;			// 256 color
 	pcx->encoding = 1;		// uncompressed
	pcx->bits_per_pixel = 8;		// 256 color
	pcx->xmin = 0;
	pcx->ymin = 0;
	pcx->xmax = LittleShort((short)(width-1));
	pcx->ymax = LittleShort((short)(height-1));
	pcx->hres = LittleShort((short)width);
	pcx->vres = LittleShort((short)height);
	Q_memset (pcx->palette,0,sizeof(pcx->palette));
	pcx->color_planes = 1;		// chunky image
	pcx->bytes_per_line = LittleShort((short)width);
	pcx->palette_type = LittleShort(2);		// not a grey scale
	Q_memset (pcx->filler,0,sizeof(pcx->filler));

// pack the image
	pack = (qbyte *)(pcx+1);
	for (i=0 ; i<height ; i++)
	{
		for (j=0 ; j<width ; j++)
		{
			if ( (*data & 0xc0) != 0xc0)
				*pack++ = *data++;
			else
			{
				*pack++ = 0xc1;
				*pack++ = *data++;
			}
		}

		data += rowbytes - width;
	}

// write the palette
	*pack++ = 0x0c;	// palette ID qbyte
	for (i=0 ; i<768 ; i++)
		*pack++ = *palette++;

// write output file
	length = pack - (qbyte *)pcx;

#ifdef HAVE_CLIENT
	if (upload)
	{
		CL_StartUpload((void *)pcx, length);
		ret = true;
	}
	else
#endif
		ret = COM_WriteFile (filename, fsroot, pcx, length);
	BZ_Free(pcx);

	return ret;
}

/*
============
LoadPCX
============
*/
qbyte *ReadPCXFile(qbyte *buf, int length, int *width, int *height)
{
	pcx_t	*pcx;
//	pcx_t pcxbuf;
	qbyte	*palette;
	qbyte	*pix;
	int		x, y;
	int		dataByte, runLength;
	int		count;
	qbyte *data;

	qbyte	*pcx_rgb;

	unsigned short xmin, ymin, swidth, sheight;

//
// parse the PCX file
//

	if (length < sizeof(*pcx))
		return NULL;

	pcx = (pcx_t *)buf;

	xmin = LittleShort(pcx->xmin);
	ymin = LittleShort(pcx->ymin);
	swidth = LittleShort(pcx->xmax)-xmin+1;
	sheight = LittleShort(pcx->ymax)-ymin+1;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| pcx->color_planes != 1
		|| swidth >= 1024
		|| sheight >= 1024)
	{
		return NULL;
	}

	*width = swidth;
	*height = sheight;

#ifdef HAVE_CLIENT
	if (r_dodgypcxfiles.value)
		palette = host_basepal;
	else
#endif
		palette = buf + length-768;

	data = (char *)(pcx+1);

	count = (swidth) * (sheight);
	pcx_rgb = BZ_Malloc( count * 4);

	for (y=0 ; y<sheight ; y++)
	{
		pix = pcx_rgb + 4*y*(swidth);
		for (x=0 ; x<swidth ; )
		{
			dataByte = *data;
			data++;

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				if (x+runLength>swidth)
				{
					Con_Printf("corrupt pcx\n");
					BZ_Free(pcx_rgb);
					return NULL;
				}
				dataByte = *data;
				data++;
			}
			else
				runLength = 1;

			while(runLength-- > 0)
			{
				pix[0] = palette[dataByte*3];
				pix[1] = palette[dataByte*3+1];
				pix[2] = palette[dataByte*3+2];
				pix[3] = 255;
				if (dataByte == 255)
				{
					pix[0] = 0;	//linear filtering can mean transparent pixel colours are visible. black is a more neutral colour.
					pix[1] = 0;
					pix[2] = 0;
					pix[3] = 0;
				}
				pix += 4;
				x++;
			}
		}
	}

	return pcx_rgb;
}

qbyte *ReadPCXData(qbyte *buf, int length, int width, int height, qbyte *result)
{
	pcx_t	*pcx;
//	pcx_t pcxbuf;
//	qbyte	*palette;
	qbyte	*pix;
	int		x, y;
	int		dataByte, runLength;
//	int		count;
	qbyte *data;

	unsigned short xmin, ymin, swidth, sheight;

//
// parse the PCX file
//

	pcx = (pcx_t *)buf;

	xmin = LittleShort(pcx->xmin);
	ymin = LittleShort(pcx->ymin);
	swidth = LittleShort(pcx->xmax)-xmin+1;
	sheight = LittleShort(pcx->ymax)-ymin+1;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8)
	{
		return NULL;
	}

	if (width != swidth ||
		height > sheight)
	{
		Con_Printf("unsupported pcx size\n");
		return NULL;	//we can't feed the requester with enough info
	}

	data = (char *)(pcx+1);

	for (y=0 ; y<height ; y++)
	{
		pix = result + y*swidth;
		for (x=0 ; x<swidth ; )
		{
			dataByte = *data;
			data++;

			if((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				if (x+runLength>swidth)
				{
					Con_Printf("corrupt pcx\n");
					return NULL;
				}
				dataByte = *data;
				data++;
			}
			else
				runLength = 1;

			while(runLength-- > 0)
			{
				*pix++ = dataByte;
				x++;
			}
		}
	}

	return result;
}

qbyte *ReadPCXPalette(qbyte *buf, int len, qbyte *out)
{
	pcx_t	*pcx;


//
// parse the PCX file
//

	pcx = (pcx_t *)buf;

	if (pcx->manufacturer != 0x0a
		|| pcx->version != 5
		|| pcx->encoding != 1
		|| pcx->bits_per_pixel != 8
		|| LittleShort(pcx->xmax) >= 1024
		|| LittleShort(pcx->ymax) >= 1024)
	{
		return NULL;
	}

	memcpy(out, (qbyte *)pcx + len - 768, 768);

	return out;
}

#endif // IMAGEFMT_PCX
