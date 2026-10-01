#include "image.h"

#ifdef IMAGEFMT_BMP

typedef struct bmpheader_s
{
	unsigned int	SizeofBITMAPINFOHEADER;
	signed int		Width;
	signed int		Height;
	unsigned short	Planes;
	unsigned short	BitCount;
	unsigned int	Compression;
	unsigned int	ImageSize;
	signed int		TargetDeviceXRes;
	signed int		TargetDeviceYRes;
	unsigned int	NumofColorIndices;
	unsigned int	NumofImportantColorIndices;
} bmpheader_t;
typedef struct bmpheaderv4_s
{
	unsigned int	RedMask;
	unsigned int	GreenMask;
	unsigned int	BlueMask;
	unsigned int	AlphaMask;
	qbyte			ColourSpace[4];	//"Win " or "sRGB"
	qbyte			ColourSpaceCrap[12*3];
	unsigned int	Gamma[3];
} bmpheaderv4_t;

static qbyte *ReadRawBMPFile(qbyte *buf, int length, int *width, int *height, size_t OffsetofBMPBits)
{
	unsigned int i;
	bmpheader_t h;
	qbyte *data;

	memcpy(&h, buf, sizeof(h));	
	h.SizeofBITMAPINFOHEADER = LittleLong(h.SizeofBITMAPINFOHEADER);
	h.Width = LittleLong(h.Width);
	h.Height = LittleLong(h.Height);
	h.Planes = LittleShort(h.Planes);
	h.BitCount = LittleShort(h.BitCount);
	h.Compression = LittleLong(h.Compression);
	h.ImageSize = LittleLong(h.ImageSize);
	h.TargetDeviceXRes = LittleLong(h.TargetDeviceXRes);
	h.TargetDeviceYRes = LittleLong(h.TargetDeviceYRes);
	h.NumofColorIndices = LittleLong(h.NumofColorIndices);
	h.NumofImportantColorIndices = LittleLong(h.NumofImportantColorIndices);

	if (h.Compression)	//RLE? BITFIELDS (gah)?
		return NULL;

	if (!OffsetofBMPBits)
		h.Height /= 2;	//icons are weird.

	*width = h.Width;
	*height = h.Height;

	if (h.BitCount == 4)	//4 bit
	{
		int x, y;
		unsigned int *data32;
		unsigned int	pal[16];
		if (!h.NumofColorIndices)
			h.NumofColorIndices = (int)pow(2, h.BitCount);
		if (h.NumofColorIndices>16)
			return NULL;
		if (h.Width&1)
			return NULL;

		data = buf;
		data += sizeof(h);

		for (i = 0; i < h.NumofColorIndices; i++)
		{
			pal[i] = data[i*4+2] + (data[i*4+1]<<8) + (data[i*4+0]<<16) + (255u/*data[i*4+3]*/<<24);
		}

		if (OffsetofBMPBits)
			buf += OffsetofBMPBits;
		else
			buf = data+h.NumofColorIndices*4;
		data32 = BZ_Malloc(h.Width * h.Height*4);
		for (y = 0; y < h.Height; y++)
		{
			i = (h.Height-1-y) * (h.Width);
			for (x = 0; x < h.Width/2; x++)
			{
				data32[i++] = pal[buf[x]>>4];
				data32[i++] = pal[buf[x]&15];
			}
			buf += (h.Width+1)>>1;
		}

		if (!OffsetofBMPBits)
		{
			for (y = 0; y < h.Height; y++)
			{
				i = (h.Height-1-y) * (h.Width);
				for (x = 0; x < h.Width; x++)
				{
					if (buf[x>>3]&(1<<(7-(x&7))))
						data32[i] &= 0x00ffffff;
					i++;
				}
				buf += (h.Width+7)>>3;
			}
		}

		return (qbyte *)data32;
	}
	else if (h.BitCount == 8)	//8 bit
	{
		int x, y;
		unsigned int *data32;
		unsigned int	pal[256];
		if (!h.NumofColorIndices)
			h.NumofColorIndices = (int)pow(2, h.BitCount);
		if (h.NumofColorIndices>256)
			return NULL;

		data = buf;
		data += sizeof(h);

		for (i = 0; i < h.NumofColorIndices; i++)
		{
			pal[i] = data[i*4+2] + (data[i*4+1]<<8) + (data[i*4+0]<<16) + (255u/*data[i*4+3]*/<<24);
		}

		if (OffsetofBMPBits)
			buf += OffsetofBMPBits;
		else
			buf += h.SizeofBITMAPINFOHEADER + h.NumofColorIndices*4;
		data32 = BZ_Malloc(h.Width * h.Height*4);
		for (y = 0; y < h.Height; y++)
		{
			i = (h.Height-1-y) * (h.Width);
			for (x = 0; x < h.Width; x++)
			{
				data32[i] = pal[buf[x]];
				i++;
			}
			//BMP rows are 32-bit aligned.
			buf += (h.Width+3)&~3;
		}

		if (!OffsetofBMPBits)
		{
			for (y = 0; y < h.Height; y++)
			{
				i = (h.Height-1-y) * (h.Width);
				for (x = 0; x < h.Width; x++)
				{
					if (buf[x>>3]&(1<<(7-(x&7))))
						data32[i] &= 0x00ffffff;
					i++;
				}
				buf += (h.Width+7)>>3;
			}
		}

		return (qbyte *)data32;
	}
	else if (h.BitCount == 24)	//24 bit... no 16?
	{
		int x, y;
		if (OffsetofBMPBits)
			buf += OffsetofBMPBits;
		else
			buf += h.SizeofBITMAPINFOHEADER;
		data = BZ_Malloc(h.Width * h.Height*4);
		for (y = 0; y < h.Height; y++)
		{
			i = (h.Height-1-y) * (h.Width);
			for (x = 0; x < h.Width; x++)
			{
				data[i*4+0] = buf[x*3+2];
				data[i*4+1] = buf[x*3+1];
				data[i*4+2] = buf[x*3+0];
				data[i*4+3] = 255;
				i++;
			}
			buf += h.Width*3;
		}

		return data;
	}
	else if (h.BitCount == 32)
	{
		int x, y;
		if (OffsetofBMPBits)
			buf += OffsetofBMPBits;
		else
			buf += h.SizeofBITMAPINFOHEADER;
		data = BZ_Malloc(h.Width * h.Height*4);
		for (y = 0; y < h.Height; y++)
		{
			i = (h.Height-1-y) * (h.Width);
			for (x = 0; x < h.Width; x++)
			{
				data[i*4+0] = buf[x*4+2];
				data[i*4+1] = buf[x*4+1];
				data[i*4+2] = buf[x*4+0];
				data[i*4+3] = buf[x*4+3];
				i++;
			}
			buf += h.Width*4;
		}

		return data;
	}
	else
		return NULL;

	return NULL;
}

qbyte *ReadBMPFile(qbyte *buf, int length, int *width, int *height)
{
	unsigned short Type				= buf[0] | (buf[1]<<8);
	unsigned short Size				= buf[2] | (buf[3]<<8) | (buf[4]<<16) | (buf[5]<<24);
//	unsigned short Reserved1		= buf[6] | (buf[7]<<8);
//	unsigned short Reserved2		= buf[8] | (buf[9]<<8);
	unsigned short OffsetofBMPBits	= buf[10] | (buf[11]<<8) | (buf[12]<<16) | (buf[13]<<24);
	if (Type != ('B'|('M'<<8)))
		return NULL;
	if (Size > length)
		return NULL;	//it got truncated at some point
	return ReadRawBMPFile(buf + 14, length-14, width, height, OffsetofBMPBits - 14);
}

qboolean WriteBMPFile(char *filename, enum fs_relative fsroot, qbyte *in, qintptr_t instride, int width, int height, uploadfmt_t fmt)
{
	int y;
	bmpheader_t h;
	bmpheaderv4_t h4;
	qbyte *data;
	qbyte *out;
	int outstride;
	int bits = 32;
	int extraheadersize = sizeof(h4);
	size_t fsize;
	qboolean success;

	memset(&h4, 0, sizeof(h4));
	h4.ColourSpace[0] = 'W';
	h4.ColourSpace[1] = 'i';
	h4.ColourSpace[2] = 'n';
	h4.ColourSpace[3] = ' ';
	switch(fmt)
	{
	case TF_RGBA32:
		h4.RedMask		= 0x000000ff;
		h4.GreenMask	= 0x0000ff00;
		h4.BlueMask		= 0x00ff0000;
		h4.AlphaMask	= 0xff000000;
		break;
	case TF_BGRA32:
		h4.RedMask		= 0x00ff0000;
		h4.GreenMask	= 0x0000ff00;
		h4.BlueMask		= 0x000000ff;
		h4.AlphaMask	= 0xff000000;
		break;
	case TF_RGBX32:
		h4.RedMask		= 0x000000ff;
		h4.GreenMask	= 0x0000ff00;
		h4.BlueMask		= 0x00ff0000;
		h4.AlphaMask	= 0x00000000;
		break;
	case TF_BGRX32:
		h4.RedMask		= 0x00ff0000;
		h4.GreenMask	= 0x0000ff00;
		h4.BlueMask		= 0x000000ff;
		h4.AlphaMask	= 0x00000000;
		break;
	case TF_RGB24:
		h4.RedMask		= 0x000000ff;
		h4.GreenMask	= 0x0000ff00;
		h4.BlueMask		= 0x00ff0000;
		h4.AlphaMask	= 0x00000000;
		bits = 3;
		break;
	case TF_BGR24:
		h4.RedMask		= 0x00ff0000;
		h4.GreenMask	= 0x0000ff00;
		h4.BlueMask		= 0x000000ff;
		h4.AlphaMask	= 0x00000000;
		bits = 3;
		extraheadersize = 0;
		break;

	default:
		return false;
	}


	outstride = width * (bits/8);
	outstride = (outstride+3)&~3;	//bmp pads rows to a multiple of 4 bytes.

//	h.Size = 14+sizeof(h)+extraheadersize + outstride*height;
//	h.Reserved1 = 0;
//	h.Reserved2 = 0;
//	h.OffsetofBMPBits = 2+sizeof(h)+extraheadersize;	//yes, this is misaligned.
	h.SizeofBITMAPINFOHEADER = (sizeof(h)-12)+extraheadersize;
	h.Width = width;
	h.Height = height;
	h.Planes = 1;
	h.BitCount = bits;
	h.Compression = extraheadersize?3/*BI_BITFIELDS*/:0/*BI_RGB aka BGR...*/;
	h.ImageSize = outstride*height;
	h.TargetDeviceXRes = 2835;//72DPI
	h.TargetDeviceYRes = 2835;
	h.NumofColorIndices = 0;
	h.NumofImportantColorIndices = 0;

	//bmp is bottom-up so flip it now.
	in += instride*(height-1);
	instride *= -1;

	fsize = 14+sizeof(h)+extraheadersize + outstride*height;	//size
	out = data = BZ_Malloc(fsize);
	//Type
	*out++ = 'B';
	*out++ = 'M';
	//Size
	*out++ = fsize&0xff;
	*out++ = (fsize>>8)&0xff;
	*out++ = (fsize>>16)&0xff;
	*out++ = (fsize>>24)&0xff;
	//Reserved1
	y = 0;
	*out++ = y&0xff;
	*out++ = (y>>8)&0xff;
	//Reserved1
	y = 0;
	*out++ = y&0xff;
	*out++ = (y>>8)&0xff;
	//OffsetofBMPBits
	y = 2+sizeof(h)+extraheadersize;	//yes, this is misaligned.
	*out++ = y&0xff;
	*out++ = (y>>8)&0xff;
	*out++ = (y>>16)&0xff;
	*out++ = (y>>24)&0xff;
	//bmpheader
	memcpy(out, &h, sizeof(h));
	out += sizeof(h);
	//v4 header
	memcpy(out, &h4, extraheadersize);
	out += extraheadersize;

	//data
	for (y = 0; y < height; y++)
	{
		memcpy(out, in, width * (bits/8));
		memset(out+width*(bits/8), 0, outstride-width*(bits/8));
		out += outstride;
		in += instride;
	}

	success = COM_WriteFile(filename, fsroot, data, fsize);
	BZ_Free(data);

	return success;
}

qbyte *ReadICOFile(const char *fname, qbyte *buf, int length, int *width, int *height, uploadfmt_t *fmt)
{
	qbyte *ret;
	size_t imgcount = buf[4] | (buf[5]<<8);
	struct
	{
		qbyte bWidth;
		qbyte bHeight;
		qbyte bColorCount;
		qbyte bReserved;
		unsigned short wPlanes;
		unsigned short wBitCount;
		unsigned short dwSize_low;
		unsigned short dwSize_high;
		unsigned short dwOffset_low;
		unsigned short dwOffset_high;
	} *img = (void*)(buf+6), *bestimg = NULL;
	size_t bestpixels = 0;
	size_t bestdepth = 0;

	//always favour the png first
	for (imgcount = buf[4] | (buf[5]<<8), img = (void*)(buf+6); imgcount-->0; img++)
	{
		size_t cc = img->wBitCount;
		size_t px = (img->bWidth?img->bWidth:256) * (img->bHeight?img->bHeight:256);
		if (!cc)	//if that was omitted, try and guess it based on raw image size. this is an over estimate.
			cc = 8 * (img->dwSize_low | (img->dwSize_high<<16)) / px;

		if (!bestimg || cc > bestdepth || (cc == bestdepth && px > bestpixels))
		{
			bestimg = img;
			bestdepth = cc;
			bestpixels = px;
		}
	}

	if (bestimg)
	{
		qbyte *indata = buf + (bestimg->dwOffset_low | (bestimg->dwOffset_high<<16));
		size_t insize = (bestimg->dwSize_low | (bestimg->dwSize_high<<16));
#ifdef AVAIL_PNGLIB
		if (insize > 4 && (indata[0] == 137 && indata[1] == 'P' && indata[2] == 'N' && indata[3] == 'G') && (ret = ReadPNGFile(fname, indata, insize, width, height, fmt, true)))
		{
			TRACE(("dbg: Read32BitImageFile: icon png\n"));
			return ret;
		}
		else
#endif
		if ((ret = ReadRawBMPFile(indata, insize, width, height, 0)))
		{
			if (fmt)
				*fmt = PTI_RGBA8;
			TRACE(("dbg: Read32BitImageFile: icon bmp\n"));
			return ret;
		}
	}

	return NULL;
}

#endif
