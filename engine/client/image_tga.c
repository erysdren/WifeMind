#include "image.h"

#ifdef IMAGEFMT_TGA

cvar_t r_dodgytgafiles = CVARD("r_dodgytgafiles", "0", "Many old glquake engines had a buggy tga loader that ignored bottom-up flags. Naturally people worked around this and the world was plagued with buggy images. Most engines have now fixed the bug, but you can reenable it if you have bugged tga files.");

typedef struct {	//cm = colourmap
	char	id_len;		//0
	char	cm_type;	//1
	qbyte	version;	//2
		char pad1;
	short	cm_idx;		//3
	short	cm_len;		//5
	char	cm_size;	//7
		char pad2;
	short	originx;	//8 (ignored)
	short	originy;	//10 (ignored)
	short	width;		//12-13
	short	height;		//14-15
	qbyte	bpp;		//16
	qbyte	attribs;	//17
} tgaheader_t;

static char *ReadGreyTargaFile (qbyte *data, int flen, tgaheader_t *tgahead, int asgrey)	//preswapped header
{
	int				columns, rows;
	int				row, column;
	qbyte			*pixbuf, *pal;
	qboolean		flipped;

	qbyte *pixels = BZ_Malloc(tgahead->width * tgahead->height * (asgrey?1:4));

	if (tgahead->version!=1
		&& tgahead->version!=3)
	{
		Con_Printf("LoadGrayTGA: Only type 1 and 3 greyscale targa images are understood.\n");
		BZ_Free(pixels);
		return NULL;
	}

	if (tgahead->version==1 && tgahead->bpp != 8 &&
		tgahead->cm_size != 24 && tgahead->cm_len != 256)
	{
		Con_Printf("LoadGrayTGA: Strange palette type\n");
		BZ_Free(pixels);
		return NULL;
	}

	columns = tgahead->width;
	rows = tgahead->height;

	flipped = !((tgahead->attribs & 0x20) >> 5);
#ifdef HAVE_CLIENT
	if (r_dodgytgafiles.value)
		flipped = true;
#endif

	if (tgahead->version == 1)
	{	//paletted data...
		pal = data;
		data += tgahead->cm_len*3;
		if (asgrey)
		{
			for(row=rows-1; row>=0; row--)
			{
				if (flipped)
					pixbuf = pixels + row*columns;
				else
					pixbuf = pixels + ((rows-1)-row)*columns;

				for(column=0; column<columns; column++)
					*pixbuf++= *data++;
			}
		}
		else
		{
			for(row=rows-1; row>=0; row--)
			{
				if (flipped)
					pixbuf = pixels + row*columns*4;
				else
					pixbuf = pixels + ((rows-1)-row)*columns*4;

				for(column=0; column<columns; column++)
				{
					*pixbuf++= pal[*data*3+2];
					*pixbuf++= pal[*data*3+1];
					*pixbuf++= pal[*data*3+0];
					*pixbuf++= 255;
					data++;
				}
			}
		}
		return pixels;
	}
	//version 3 now. pure greyscale

	if (asgrey)
	{
		for(row=rows-1; row>=0; row--)
		{
			if (flipped)
				pixbuf = pixels + row*columns;
			else
				pixbuf = pixels + ((rows-1)-row)*columns;

			pixbuf = pixels + row*columns;
			for(column=0; column<columns; column++)
				*pixbuf++= *data++;
		}
	}
	else
	{
		for(row=rows-1; row>=0; row--)
		{
			if (flipped)
				pixbuf = pixels + row*columns*4;
			else
				pixbuf = pixels + ((rows-1)-row)*columns*4;

			for(column=0; column<columns; column++)
			{
				*pixbuf++= *data;
				*pixbuf++= *data;
				*pixbuf++= *data;
				*pixbuf++= 255;
				data++;
			}
		}
	}

	return pixels;
}

#define MISSHORT(ptr) (*(ptr) | (*(ptr+1) << 8))
//remember to free it
//greyonly causes the function to fail if given anything but greyscale images
//this is for detecting heightmaps instead of normalmaps.
void *ReadTargaFile(qbyte *buf, int length, int *width, int *height, uploadfmt_t *format, qboolean greyonly, uploadfmt_t forceformat)
{
	//tga files sadly lack a true magic header thing.
	unsigned char *data;

	qboolean flipped;

	tgaheader_t tgaheader;	//things are misaligned, so no pointer.

	if (length < 18 || buf[1] > 1)
		return NULL;	//probably not a tga...

	tgaheader.id_len = buf[0];
	tgaheader.cm_type = buf[1];
	tgaheader.version = buf[2];
	tgaheader.cm_idx = MISSHORT(buf+3);
	tgaheader.cm_len = MISSHORT(buf+5);
	tgaheader.cm_size = buf[7];
	tgaheader.originx = MISSHORT(buf+8);
	tgaheader.originy = MISSHORT(buf+10);
	tgaheader.width = MISSHORT(buf+12);
	tgaheader.height = MISSHORT(buf+14);
	tgaheader.bpp = buf[16];
	tgaheader.attribs = buf[17];

	switch(tgaheader.version)
	{
	case 0:	//No image data included.
		return NULL;	//not really valid for us. reject it after all
	case 1:	//Uncompressed, color-mapped images.
	case 2:	//Uncompressed, RGB images.
	case 3:	//Uncompressed, black and white images.
	case 9:	//Runlength encoded color-mapped images.
	case 10:	//Runlength encoded RGB images.
	case 11:	//Runlength encoded, black and white images.
	case 32:	//Compressed color-mapped data, using Huffman, Delta, and runlength encoding.
	case 33:	//Compressed color-mapped data, using Huffman, Delta, and runlength encoding.  4-pass quadtree-type process.
		if (buf[16] != 8 && buf[16] != 16 && buf[16] != 24 && buf[16] != 32)
			return NULL;	//unsupported bitdepths
		break;
	//0x80+ are third-party extensions...
	case 0x82:	//half-float rgb
	case 0x83:	//half-float greyscale
		if (forceformat != PTI_INVALID)
			return NULL;
		if ((buf[16]&15) || buf[16]<16 || buf[16] > 16*4)
			return NULL;	//unsupported bitdepths
		break;
	default:
		return NULL;
	}
	//validate the size to some sanity limit.
	if ((unsigned short)tgaheader.width > 16384 || (unsigned short)tgaheader.height > 16384)
		return NULL;


	flipped = !((tgaheader.attribs & 0x20) >> 5);
#ifdef HAVE_CLIENT
	if (r_dodgytgafiles.value)
		flipped = true;
#endif

	data=buf+18;
	data += tgaheader.id_len;

	*width = tgaheader.width;
	*height = tgaheader.height;

	if (greyonly)	//grey only, load as 8 bit..
	{
		if (!(tgaheader.version == 1) && !(tgaheader.version == 3) && !(tgaheader.version == 11))
			return NULL;
	}
	if (tgaheader.version == 1 || tgaheader.version == 3)
	{
		if (forceformat==PTI_L8 || forceformat==PTI_RGBA8 || forceformat==PTI_RGBX8)
			*format = forceformat;
		else if (tgaheader.version == 3)
			*format = PTI_L8;
		else
			*format = PTI_RGBX8;
		return ReadGreyTargaFile(data, length, &tgaheader, *format==PTI_L8);
	}
	else if (tgaheader.version == 10 || tgaheader.version == 9 || tgaheader.version == 11)
	{
		//9:RLE paletted
		//10:RLE bgr(a)
		//11:RLE greyscale
#undef getc
#define getc(x) *data++
		unsigned int row, rows=tgaheader.height, column, columns=tgaheader.width, packetHeader, packetSize, j;
		qbyte *pixbuf, *targa_rgba;
		unsigned int inraw;

		qbyte blue, red, green, alphabyte;

		byte_vec4_t palette[256];
		enum
		{
			rle_p8,
			rle_a1rgb5,
			rle_bgr8,
			rle_bgra8,
			rle_l8,
			rle_l8a8,
		} rlemode;
		int outbytes;

		*format = PTI_RGBX8;
		if (tgaheader.version == 9)
		{	//RLE palette
			if (tgaheader.bpp == 8)	//FIXME: tgaheader.bpp can be 8, 15, or 16.
				rlemode = rle_p8;
			else
				return NULL;

			for (row = 0; row < 256; row++)
			{
				palette[row][0] = row;
				palette[row][1] = row;
				palette[row][2] = row;
				palette[row][3] = 255;
			}

			if (forceformat == PTI_L8 || forceformat == PTI_INVALID)
				*format = forceformat = PTI_L8;
			else
				*format = forceformat = PTI_LLLX8;


			if (tgaheader.cm_type)
			{
				qboolean grey = true;
				switch(tgaheader.cm_size)
				{
				default:
					return NULL;
				case 24:
					for (row = 0; row < tgaheader.cm_len; row++)
					{
						if (data[0] != data[1] || data[0] != data[2])
							grey = false;
						palette[row][0] = *data++;
						palette[row][1] = *data++;
						palette[row][2] = *data++;
						palette[row][3] = 255;
					}
					if (grey && forceformat == PTI_INVALID)
						*format = PTI_L8;
					else if (forceformat != PTI_L8)
						*format = PTI_RGBA8;
					break;
				case 32:
					for (row = 0; row < tgaheader.cm_len; row++)
					{
						if (data[0] != data[1] || data[0] != data[2] || data[3] != 0xff)
							grey = false;
						palette[row][0] = *data++;
						palette[row][1] = *data++;
						palette[row][2] = *data++;
						palette[row][3] = *data++;
					}
					if (grey && forceformat == PTI_INVALID)
						*format = PTI_L8;
					else if (forceformat != PTI_L8)
						*format = PTI_RGBA8;
					break;
				}
			}
		}
		else if (tgaheader.version == 10)
		{	//RLE truecolour
			if (tgaheader.bpp == 16)
				rlemode = rle_a1rgb5;
			else if (tgaheader.bpp == 24)
				rlemode = rle_bgr8;
			else if (tgaheader.bpp == 32)
				rlemode = rle_bgra8;
			else
				return NULL;

			*format = (tgaheader.bpp==24)?PTI_RGBX8:PTI_RGBA8;
		}
		else if (tgaheader.version == 11)
		{	//RLE greyscale
			if (tgaheader.bpp == 8)
				rlemode = rle_l8;
			else if (tgaheader.bpp == 16)
				rlemode = rle_l8a8;
			else
				return NULL;

			if (forceformat == PTI_L8)
				*format = forceformat;
			else if (rlemode==rle_l8a8)
				*format = PTI_LLLA8; //should probably use PTI_L8A8, but the caller will know to optimise
			else
				*format = PTI_LLLX8;
		}
		else
			return NULL;

		if (*format == PTI_L8)
			outbytes = 1;
		else if (*format == PTI_L8A8)
			outbytes = 2;
		else
			outbytes = 4;
		targa_rgba=BZ_Malloc(rows*columns*outbytes);
		for(row=rows; row-->0; )
		{
			if (flipped)
				pixbuf = targa_rgba + row*columns*outbytes;
			else
				pixbuf = targa_rgba + ((rows-1)-row)*columns*outbytes;
			for(column=0; column<columns; )
			{
				packetHeader=*data++;
				packetSize = 1 + (packetHeader & 0x7f);
				if (packetHeader & 0x80)
				{	// run-length packet
					switch (rlemode)
					{
					case rle_p8:
						blue = palette[*data][0];
						green = palette[*data][1];
						red = palette[*data][2];
						alphabyte = palette[*data][3];
						data++;
						break;
					case rle_l8a8:
						blue = green = red = *data++;
						alphabyte = *data++;
						break;
					case rle_l8:
						blue = green = red = *data++;
						alphabyte = 255;
						break;

					case rle_a1rgb5:
						inraw = data[0] | (data[1]<<8);
						data+=2;
						alphabyte = (inraw&0x8000)?255:0;
						red = ((inraw>>10)&0x1f)<<3;
						green = ((inraw>>5)&0x1f)<<3;
						blue = ((inraw>>0)&0x1f)<<3;
						break;
					case rle_bgr8:
						blue = *data++;
						green = *data++;
						red = *data++;
						alphabyte = 255;
						break;
					case rle_bgra8:
						blue = *data++;
						green = *data++;
						red = *data++;
						alphabyte = *data++;
						break;
					default:
						blue = green = red = alphabyte = 255;
						break;
					}

					if (*format!=PTI_L8)	//keep colours
					{
						for(j=0;j<packetSize;j++)
						{
							*pixbuf++=red;
							*pixbuf++=green;
							*pixbuf++=blue;
							*pixbuf++=alphabyte;
							column++;
							if (column==columns)
							{ // run spans across rows
								column=0;
								if (row>0)
									row--;
								else
									goto breakOut;
								if (flipped)
									pixbuf = targa_rgba + row*columns*4;
								else
									pixbuf = targa_rgba + ((rows-1)-row)*columns*4;
							}
						}
					}
					else	//convert to greyscale
					{
						for(j=0;j<packetSize;j++)
						{
							*pixbuf++ = red*NTSC_RED + green*NTSC_GREEN + blue*NTSC_BLUE;
							column++;
							if (column==columns)
							{ // run spans across rows
								column=0;
								if (row>0)
									row--;
								else
									goto breakOut;
								if (flipped)
									pixbuf = targa_rgba + row*columns*1;
								else
									pixbuf = targa_rgba + ((rows-1)-row)*columns*1;
							}
						}
					}
				}
				else
				{                            // non run-length packet
					if (*format!=PTI_L8)	//keep colours
					{
						for(j=0;j<packetSize;j++)
						{
							switch (rlemode)
							{
							case rle_p8:
								blue = palette[*data][0];
								green = palette[*data][1];
								red = palette[*data][2];
								alphabyte = palette[*data][3];
								data++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = alphabyte;
								break;
							case rle_l8a8:
								blue = green = red = *data++;
								alphabyte = *data++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = alphabyte;
								break;
							case rle_l8:
								blue = green = red = *data++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = 255;
								break;
							case rle_a1rgb5:
								inraw = data[0] | (data[1]<<8);
								data+=2;
								alphabyte = (inraw&0x8000)?255:0;
								red = ((inraw>>10)&0x1f)<<3;
								green = ((inraw>>5)&0x1f)<<3;
								blue = ((inraw>>0)&0x1f)<<3;

								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = alphabyte;
								break;
							case rle_bgr8:
								blue = *data++;
								green = *data++;
								red = *data++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = 255;
								break;
							case rle_bgra8:
								blue = *data++;
								green = *data++;
								red = *data++;
								alphabyte = *data++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = alphabyte;
								break;
							}
							column++;
							if (column==columns)
							{ // pixel packet run spans across rows
								column=0;
								if (row>0)
									row--;
								else
									goto breakOut;
								if (flipped)
									pixbuf = targa_rgba + row*columns*4;
								else
									pixbuf = targa_rgba + ((rows-1)-row)*columns*4;
							}
						}
					}
					else	//convert to grey
					{
						for(j=0;j<packetSize;j++)
						{
							switch (rlemode)
							{
							case rle_p8:
								blue = palette[*data][0];
								green = palette[*data][1];
								red = palette[*data][2];
								*pixbuf++ = (blue + green + red)/3;
								data++;
								break;
							case rle_l8a8:
								blue = green = red = *data++;
								alphabyte = *data++;
								*pixbuf++ = green;
								break;
							case rle_l8:
								blue = green = red = *data++;
								*pixbuf++ = green;
								break;
							case rle_a1rgb5:
								inraw = data[0] | (data[1]<<8);
								data+=2;
								alphabyte = (inraw&0x8000)?255:0;
								red = ((inraw>>10)&0x1f)<<3;
								green = ((inraw>>5)&0x1f)<<3;
								blue = ((inraw>>0)&0x1f)<<3;

								*pixbuf++ = red*NTSC_RED + green*NTSC_GREEN + blue*NTSC_BLUE;
								break;
							case rle_bgr8:
								blue = *data++;
								green = *data++;
								red = *data++;
								*pixbuf++ = red*NTSC_RED + green*NTSC_GREEN + blue*NTSC_BLUE;
								break;
							case rle_bgra8:
								blue = *data++;
								green = *data++;
								red = *data++;
								alphabyte = *data++;
								*pixbuf++ = red*NTSC_RED + green*NTSC_GREEN + blue*NTSC_BLUE;
								break;
							}
							column++;
							if (column==columns)
							{ // pixel packet run spans across rows
								column=0;
								if (row>0)
									row--;
								else
									goto breakOut;
								if (flipped)
									pixbuf = targa_rgba + row*columns*1;
								else
									pixbuf = targa_rgba + ((rows-1)-row)*columns*1;
							}
						}
					}
				}
			}
		}
		breakOut:;

		return targa_rgba;
	}
	else if ((tgaheader.version == 0x82||tgaheader.version == 0x83) && forceformat && forceformat!=PTI_RGBA16F)
		Con_Printf("HTGA: required output format is not half-float\n");
	else if ((tgaheader.version == 0x82||tgaheader.version == 0x83) && !(tgaheader.bpp&15) && tgaheader.bpp>=16 && tgaheader.bpp<=16*4)
	{	//packed r[g[b[a]]]f
		unsigned short *initbuf, *inrow, *outrow;
		int x, y, mul;

		if (tgaheader.version == 0x83 && tgaheader.bpp==16)
			*format = forceformat = PTI_R16F;
		else
			*format = forceformat = PTI_RGBA16F; //gray+alpha needs to be rgbaf

		initbuf = BZ_Malloc(tgaheader.height*tgaheader.width* ((forceformat==PTI_R16F)?2:8));

		mul = tgaheader.bpp/8;
//flip +convert to 32 bit
		outrow = &initbuf[(int)(0)*tgaheader.width*mul];
		for (y = 0; y < tgaheader.height; y+=1)
		{
			if (flipped)
				inrow = (unsigned short*)&data[(int)(tgaheader.height-y-1)*tgaheader.width*mul];
			else
				inrow = (unsigned short*)&data[(int)(y)*tgaheader.width*mul];

			switch(mul)
			{
			default:	//bug!
				for (x = 0; x < tgaheader.width; x+=1)
				{
					*outrow++ = 0;
					*outrow++ = 0;
					*outrow++ = 0;
					*outrow++ = 0xf<<10;
				}
				break;
			case 2:	//Lum
				if (forceformat == PTI_R16F)
				{
					for (x = 0; x < tgaheader.width; x+=1)
						*outrow++ = *inrow++;
				}
				else
				{
					for (x = 0; x < tgaheader.width; x+=1)
					{
						*outrow++ = *inrow;
						*outrow++ = *inrow;
						*outrow++ = *inrow;
						*outrow++ = 0xf<<10; //1.0
						inrow+=1;
					}
				}
				break;
			case 4:
				if (tgaheader.version == 0x83)
				{	//treat as LumAlpha
					for (x = 0; x < tgaheader.width; x+=1)
					{
						*outrow++ = inrow[0];
						*outrow++ = inrow[0];
						*outrow++ = inrow[0];
						*outrow++ = inrow[1];
						inrow+=2;
					}
				}
				else
				{	//RG
					for (x = 0; x < tgaheader.width; x+=1)
					{
						*outrow++ = inrow[0];
						*outrow++ = inrow[1];
						*outrow++ = 0;
						*outrow++ = 0xf<<10; //1.0
						inrow+=2;
					}
				}
				break;
			case 6: //BGR
				for (x = 0; x < tgaheader.width; x+=1)
				{
					*outrow++ = inrow[2];
					*outrow++ = inrow[1];
					*outrow++ = inrow[0];
					*outrow++ = 0xf<<10; //1.0
					inrow+=3;
				}
				break;
			case 8: //BGRA16F, swizzle to rgba
				for (x = 0; x < tgaheader.width; x+=1)
				{
					*outrow++ = inrow[2];
					*outrow++ = inrow[1];
					*outrow++ = inrow[0];
					*outrow++ = inrow[3];
					inrow+=4;
				}
				break;
			}
		}
		return initbuf;
	}
	else if (tgaheader.version == 2)
	{	//packed format
		qbyte *initbuf, *inrow, *outrow;
		int x, y, mul;
		qbyte blue, red, green;

		if (tgaheader.bpp == 8)
			return NULL;
		initbuf=BZ_Malloc(tgaheader.height*tgaheader.width* ((forceformat==PTI_L8)?1:4));

		mul = tgaheader.bpp/8;
//flip +convert to 32 bit
		if (forceformat==PTI_L8)
		{
			*format = forceformat;
			outrow = &initbuf[(int)(0)*tgaheader.width];
		}
		else
		{
			outrow = &initbuf[(int)(0)*tgaheader.width*mul];
			*format = (mul==4)?PTI_RGBA8:PTI_RGBX8;
		}
		for (y = 0; y < tgaheader.height; y+=1)
		{
			if (flipped)
				inrow = &data[(int)(tgaheader.height-y-1)*tgaheader.width*mul];
			else
				inrow = &data[(int)(y)*tgaheader.width*mul];

			if (forceformat!=PTI_L8)
			{
				switch(mul)
				{
				case 2:
					for (x = 0; x < tgaheader.width; x+=1)
					{
						*outrow++ = ((inrow[1] & 0x7c)>>2) *8;					//red
						*outrow++ = (((inrow[1] & 0x03)<<3) + ((inrow[0] & 0xe0)>>5))*8;	//green
						*outrow++ = (inrow[0] & 0x1f)*8;					//blue
						*outrow++ = (int)(inrow[1]&0x80)*2-1;			//alpha?
						inrow+=2;
					}
					break;
				case 3:
					for (x = 0; x < tgaheader.width; x+=1)
					{
						*outrow++ = inrow[2];
						*outrow++ = inrow[1];
						*outrow++ = inrow[0];
						*outrow++ = 255;
						inrow+=3;
					}
					break;
				case 4:
					for (x = 0; x < tgaheader.width; x+=1)
					{
						*outrow++ = inrow[2];
						*outrow++ = inrow[1];
						*outrow++ = inrow[0];
						*outrow++ = inrow[3];
						inrow+=4;
					}
					break;
				}
			}
			else
			{
				switch(mul)
				{
				case 2:
					for (x = 0; x < tgaheader.width; x+=1)
					{
						red = ((inrow[1] & 0x7c)>>2) *8;					//red
						green = (((inrow[1] & 0x03)<<3) + ((inrow[0] & 0xe0)>>5))*8;	//green
						blue = (inrow[0] & 0x1f)*8;					//blue
//						alphabyte = (int)(inrow[1]&0x80)*2-1;			//alpha?

						*outrow++ = red*NTSC_RED + green*NTSC_GREEN + blue*NTSC_BLUE;
						inrow+=2;
					}
					break;
				case 3:
					for (x = 0; x < tgaheader.width; x+=1)
					{
						red = inrow[2];
						green = inrow[1];
						blue = inrow[0];
						*outrow++ = red*NTSC_RED + green*NTSC_GREEN + blue*NTSC_BLUE;
						inrow+=3;
					}
					break;
				case 4:
					for (x = 0; x < tgaheader.width; x+=1)
					{
						red = inrow[2];
						green = inrow[1];
						blue = inrow[0];
						*outrow++ = red*NTSC_RED + green*NTSC_GREEN + blue*NTSC_BLUE;
						inrow+=4;
					}
					break;
				}
			}
		}

		if (forceformat!=PTI_L8)
		{
			for (x = 0; x < tgaheader.width*tgaheader.height*4; x+=4)
			{
				if (initbuf[x+0] != initbuf[x+1] || initbuf[x+0] != initbuf[x+2] || initbuf[x+3] != 0xff)
					break;
			}
			if (x == tgaheader.width*tgaheader.height*4)
			{	//no alpha
				if (forceformat==PTI_INVALID)
					*format = PTI_LLLX8;
			}
			else
			{
				for (; x < tgaheader.width*tgaheader.height*4; x+=4)
				{
					if (initbuf[x+0] != initbuf[x+1] || initbuf[x+0] != initbuf[x+2])
						break;
				}
				if (x == tgaheader.width*tgaheader.height*4)
				{	//okay, there's some alpha data in there.
					if (forceformat==PTI_INVALID)
						*format = PTI_LLLA8;
				}
			}
		}
		return initbuf;
	}
	else
		Con_Printf("TGA: Unsupported version\n");
	return NULL;
}

qboolean WriteTGA(const char *filename, enum fs_relative fsroot, const qbyte *fte_restrict rgb_buffer, qintptr_t bytestride, int width, int height, enum uploadfmt fmt)
{
	qboolean success = false;
	size_t c, i;
	vfsfile_t *vfs;
	int ipx,opx;
	qboolean rgb;
	static const unsigned char footer[26] =
	{	//added in v2, just makes it clear that it is actually a tga
		0,0,0,0,//extension area offset
		0,0,0,0,//developer area offset
		'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E', //the truth is out there
		'.', 0
	};
	unsigned char header[18];
	memset (header, 0, 18);

	if (fmt == PTI_RGBA16F)
	{
		header[2] = 0x82;	//uncompressed RGB half-float
		opx = 8;
		ipx = 8;
		rgb = true;
	}
	else if (fmt == PTI_R16F)
	{
		header[2] = 0x83;	//uncompressed greyscale half-float
		opx = 2;
		ipx = 2;
		rgb = false;
	}
	else
	{
		header[2] = 2;			// uncompressed true-colour type
		if (fmt == PTI_ARGB1555)
		{
			rgb = false;
			ipx = 2;
			opx = 2;
			header[17] |= 1&0xf;	//1bit alpha.
		}
		else if (fmt == PTI_RGBA8 || fmt == PTI_BGRA8)
		{
			rgb = fmt==TF_RGBA32;
			ipx = 4;
			opx = 4;
			header[17] |= 8&0xf;	//alpha is 8bit
		}
		else if (fmt == PTI_RGBX8 || fmt == PTI_BGRX8)
		{
			rgb = fmt==PTI_RGBX8;
			ipx = 4;
			opx = 3;
		}
		else if (fmt == PTI_RGB8 || fmt == PTI_BGR8)
		{
			rgb = fmt==PTI_RGB8;
			ipx = 3;
			opx = 3;
		}
		/*else if (fmt == PTI_RGBA16)
		{
			rgb = true;
			ipx = 8;
			opx = 8;
		}*/
		else if (fmt==PTI_LLLA8)
		{
			rgb = false;
			ipx = 4;
			opx = 2;
			header[2] = 3;			// greyscale
			header[17] |= 8&0xf;			// with alpha
		}
		else if (fmt==PTI_LLLX8)
		{
			rgb = false;
			ipx = 4;
			opx = 1;
			header[2] = 3;			// greyscale
		}
		else if (fmt == PTI_L8)
		{
			rgb = false;
			ipx = 1;
			opx = 1;
			header[2] = 3;			// greyscale
		}
		else if (fmt == PTI_L8A8)
		{
			rgb = false;
			ipx = 2;
			opx = 2;
			header[2] = 3;			// greyscale
			header[17] |= 8&0xf;			//with alpha
		}
		else
			return false;
	}

	FS_CreatePath(filename, fsroot);
	vfs = FS_OpenVFS(filename, "wb", fsroot);
	if (vfs)
	{
		header[12] = width&255;
		header[13] = width>>8;
		header[14] = height&255;
		header[15] = height>>8;
		header[16] = opx*8;		// pixel size
		header[17] |= 0x00;		// flags

		if (bytestride < 0)
		{	//if we're upside down, lets just use an upside down tga.
			rgb_buffer += bytestride*(height-1);
			bytestride = -bytestride;
			//now we can just do everything without worrying about rows
		}
		else	//our data is top-down, set up the header to also be top-down.
			header[17] |= 0x20;

		VFS_WRITE(vfs, header, sizeof(header));
		if (ipx == opx && !rgb)
		{	//can just directly write it
			//bgr24, bgra24
			c = (size_t)width*height*opx;

			VFS_WRITE(vfs, rgb_buffer, c);
		}
		else
		{
			qbyte *fte_restrict rgb_out = malloc((size_t)width*opx*height);

			if (opx == 1)
			{	//L8, LLLX8
				c = (size_t)width*height;
				for (i=0 ; i<c ; i++)
					rgb_out[i] = rgb_buffer[i*ipx+0];
			}
			else if (opx == 2)
			{	//L8A8, LLLA8
				c = (size_t)width*height;
				for (i=0 ; i<c ; i++)
				{
					rgb_out[i*2+0] = rgb_buffer[i*ipx+0];
					rgb_out[i*2+1] = rgb_buffer[i*ipx+ipx-1];
				}
			}
			//no need to swap alpha, and if we're just swapping alpha will be fine in-place.
			else if (opx == 8)
			{	//rgba16, rgba16f
				//(output is bgra still)
				c = (size_t)width*height;
				for (i=0 ; i<c ; i++)
				{
					rgb_out[i*opx+0] = rgb_buffer[i*ipx+4];
					rgb_out[i*opx+1] = rgb_buffer[i*ipx+5];
					rgb_out[i*opx+2] = rgb_buffer[i*ipx+2];
					rgb_out[i*opx+3] = rgb_buffer[i*ipx+3];
					rgb_out[i*opx+4] = rgb_buffer[i*ipx+0];
					rgb_out[i*opx+5] = rgb_buffer[i*ipx+1];
					rgb_out[i*opx+6] = rgb_buffer[i*ipx+6];
					rgb_out[i*opx+7] = rgb_buffer[i*ipx+7];
				}
			}
			else if (opx == 4)
			{	//rgba32, bgra32
				int rc = rgb?0:2;
				int bc = rgb?2:0;
				c = (size_t)width*height;
				for (i=0 ; i<c ; i++)
				{
					rgb_out[i*4+0] = rgb_buffer[i*ipx+bc];
					rgb_out[i*4+1] = rgb_buffer[i*ipx+1];
					rgb_out[i*4+2] = rgb_buffer[i*ipx+rc];
					rgb_out[i*4+3] = rgb_buffer[i*ipx+3];
				}
			}
			else //if (opx == 3)
			{	//rgba32, bgra32
				int rc = rgb?0:2;
				int bc = rgb?2:0;
				c = (size_t)width*height;
				for (i=0 ; i<c ; i++)
				{
					rgb_out[i*3+0] = rgb_buffer[i*ipx+bc];
					rgb_out[i*3+1] = rgb_buffer[i*ipx+1];
					rgb_out[i*3+2] = rgb_buffer[i*ipx+rc];
				}
			}
			c *= opx;

			VFS_WRITE(vfs, rgb_out, c);
			free(rgb_out);
		}
		VFS_WRITE(vfs, footer, sizeof(footer));

		success = VFS_CLOSE(vfs);
	}
	return success;
}

#endif // IMAGEFMT_TGA

