#include "image.h"

#ifdef IMAGEFMT_PSD			//baselayer only.

// https://www.adobe.com/devnet-apps/photoshop/fileformatashtml/
struct psdctx_s
{
	qbyte *buf;
	qbyte *end;
};
static qbyte PSD_Byte(struct psdctx_s *ctx)
{
	if (ctx->buf == ctx->end)
		return 0;
	return *ctx->buf++;
}
static unsigned short PSD_UShort(struct psdctx_s *ctx)
{
	return (PSD_Byte(ctx)<<8)|PSD_Byte(ctx);
}
static unsigned int PSD_UInt(struct psdctx_s *ctx)
{
	return (PSD_Byte(ctx)<<24)|(PSD_Byte(ctx)<<16)|(PSD_Byte(ctx)<<8)|PSD_Byte(ctx);
}
static void *PSD_Block(struct psdctx_s *ctx, size_t sz)
{
	void *r;
	if (ctx->buf+sz <= ctx->end)
	{
		r = ctx->buf;
		ctx->buf += sz;
	}
	else
		r = NULL;
	return r;
}
void *ReadPSDFile(qbyte *buf, size_t len, const char *fname, int *outwidth, int *outheight, uploadfmt_t *outformat)
{
	unsigned short ver, chans, depth, clrmode, cmp;
	unsigned int width, height, clrsize, ressize, lyrsize;
	struct psdctx_s ctx;
	size_t l, c, y;
	ctx.buf = buf;
	ctx.end = buf+len;

	/*magic	=*/ PSD_UInt(&ctx);
	ver = PSD_UShort(&ctx);
	if (ver != 1)
	{
		Con_Printf("%s unsupported .psd version\n", fname);
		return NULL;
	}
	/*reserved*/PSD_Block(&ctx, 6);
	chans   = PSD_UShort(&ctx);
	width   = PSD_UInt(&ctx);
	height  = PSD_UInt(&ctx);
	depth   = PSD_UShort(&ctx);
	clrmode = PSD_UShort(&ctx);
	clrsize = PSD_UInt(&ctx);
	/*palette =*/ PSD_Block(&ctx, clrsize);
	ressize = PSD_UInt(&ctx);
	/*resdata =*/ PSD_Block(&ctx, ressize);
	lyrsize = PSD_UInt(&ctx);
	/*lyrdata =*/ PSD_Block(&ctx, lyrsize);
	cmp		= PSD_UShort(&ctx);

	if (width <= 0 || height <= 0 || width > 16384 || height > 16384)
	{
		Con_Printf("%s too large dimensions (%u * %u)\n", fname, width, height);
		return NULL;
	}
	if (chans <= 0)
	{
		Con_Printf("%s has no colour channels\n", fname);
		return NULL;
	}
	if (clrmode == 3 && (depth == 8 || depth == 16)) //RGB
		;
	else
	{
		Con_Printf("%s not 8 or 16bpp RGB .psd image\n", fname);
		return NULL;
	}

	//the data is planer
	if (cmp == 0 || (cmp == 1 && depth==8))
	{
		if (cmp)
			PSD_Block(&ctx, 2*chans*height); //2 byte run size per plane*scanline
		if (depth == 16)
		{
			unsigned short *r, *o;
			r = o = BZ_Malloc(sizeof(*o)*4*width*height);
			for(c = 0; c < 4; c++)
			{
				o = r+c;
				if (c < chans)
				{
					for(l = 0; l < width*height; l++, o+=4)
						*o = PSD_UShort(&ctx);
				}
				else
				{	//pad colour to 0, alpha 1
					for(l = 0; l < width*height; l++, o+=4)
						*o = (c==3)?0xffff:0;
				}
			}
			*outwidth = width;
			*outheight = height;
			*outformat = PTI_RGBA16;
			return r;
		}
		else if (depth == 8)
		{
			qbyte *r, *o;
			r = o = BZ_Malloc(sizeof(*o)*4*width*height);
			for(c = 0; c < 4; c++)
			{
				o = r+c;
				if (c < chans)
				{
					if (cmp == 1)
					{
						for(y = 0; y < height; y++)
						{
							for(l = 0; l < width; )
							{
								qbyte run = PSD_Byte(&ctx), val;
								if (run < 128)
								{	//copy
									run++;
									for (; l < width && run --> 0; l++, o+=4)
										*o = PSD_Byte(&ctx);
								}
								else
								{
									run = 1-(char)run;
									val = PSD_Byte(&ctx);
									for (; l < width && run --> 0; l++, o+=4)
										*o = val;
								}
							}
						}
					}
					else for(l = 0; l < width*height; l++, o+=4)
						*o = PSD_Byte(&ctx);
				}
				else
				{	//pad colour to 0, alpha 1
					for(l = 0; l < width*height; l++, o+=4)
						*o = (c==3)?0xff:0;
				}
			}
			*outwidth = width;
			*outheight = height;
			if (chans >= 4)
				*outformat = PTI_RGBA8;
			else
				*outformat = PTI_RGBX8;
			return r;
		}
	}
	else if (cmp == 1)
	{
	}
	Con_Printf("%s unsupported compression type (%u)\n", fname, cmp);
	return NULL;
}

#endif // IMAGEFMT_PSD
