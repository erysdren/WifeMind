#include "image.h"

#ifdef IMAGEFMT_XCF

struct xcf_s
{
	const qbyte *filestart;
	qofs_t filesize;
	qofs_t offset;

	unsigned int version;
	qbyte compression;
	size_t width;
	size_t height;
	int basetype, precision;

	enum uploadfmt outformat;
	size_t numlayers;
	size_t *layeroffsets;
	qbyte *flat;
};
static size_t XCF_ReadData(struct xcf_s *f, qbyte *out, size_t len)
{
	size_t avail;
	if (f->offset >= f->filesize)
		avail = 0;
	else
		avail = f->filesize - f->offset;
	if (len > avail)
		memset(out+avail, 0, len-avail);
	else
		avail = len;
	memcpy(out, f->filestart+f->offset, avail);
	f->offset += avail;
	return avail;
}
static unsigned char XCF_ReadByte(struct xcf_s *f)
{
	if (f->offset >= f->filesize)
		return 0;
	return f->filestart[f->offset++];
}
static unsigned int XCF_Read32(struct xcf_s *f)
{	//XCF is natively big-endian.
	qbyte w[4];
	XCF_ReadData(f, w, sizeof(w));
	return (w[0]<<24)|(w[1]<<16)|(w[2]<<8)|(w[3]<<0);
}
static float XCF_ReadFloat(struct xcf_s *f)
{	//XCF is natively big-endian.
	union
	{
		unsigned int u;
		float f;
	} u;
	qbyte w[4];
	XCF_ReadData(f, w, sizeof(w));
	u.u = (w[0]<<24)|(w[1]<<16)|(w[2]<<8)|(w[3]<<0);
	return u.f;
}
static qofs_t XCF_ReadOffset(struct xcf_s *f)
{	//XCF is natively big-endian.
	qbyte w[8];
	size_t h, l;
	if (f->version >= 11)
	{
		XCF_ReadData(f, w, sizeof(w));
		h = (w[0]<<24)|(w[1]<<16)|(w[2]<<8)|(w[3]<<0);
		l = (w[4]<<24)|(w[5]<<16)|(w[6]<<8)|(w[7]<<0);
		return qofs_Make(l, h);
	}
	return XCF_Read32(f);
}
static size_t XCF_ReadString(struct xcf_s *f, char *out, size_t outsize)
{
	size_t len = XCF_Read32(f);
	if (!len)
		*out = 0;
	else
	{
		if (outsize > len)
			outsize = len;
		outsize--;
		XCF_ReadData(f, out, outsize);
		out[outsize]=0;
		f->offset += len-outsize;
	}
	return len;
}
static void XCF_ReadHeaderProperties(struct xcf_s *f)
{	//XCF is natively big-endian.
	unsigned int proptype, propsize;

	for(;;)
	{
		proptype = XCF_Read32(f);
		propsize = XCF_Read32(f);
		if (!proptype)
			break;
		else if (proptype == 17)	//compression
			XCF_ReadData(f, &f->compression,1);
		else if (proptype == 19)	//prop_resolution
			f->offset += propsize;
		else if (proptype == 20)	//tatoo
			f->offset += propsize;
		else if (proptype == 21)	//parasites
			f->offset += propsize;
		else if (proptype == 22)	//prop_unit
			f->offset += propsize;
		else
		{
			Con_Printf("Unknown image property %i\n", proptype);
			f->offset += propsize;
		}
	}
}
static qboolean XCF_ReadTile(struct xcf_s *f, qbyte *out, int pxsize, int bytestride, int bytew, int h)
{
	if (f->compression==0)
	{
		while(h --> 0)
		{
			XCF_ReadData(f, out, bytew);
			out += bytestride;
		}
		return true;
	}
	else if (f->compression == 1)
	{
		int runsize;
		int runtype;
		int runval;
		int rowbyte;
		int c = 0, y;
		qbyte *outdata = out;
		//data is ordered by channels
nextchan:
		if (c == pxsize)
			return true;
		out = outdata+c;
		rowbyte = 0;
		c++;
		for(y=0;;)
		{
			runsize = XCF_ReadByte(f);
			if (runsize <= 127)
			{	//compressed run
				if (runsize == 127)
				{
					runsize = XCF_ReadByte(f)<<8;
					runsize |= XCF_ReadByte(f);
				}
				else
					runsize++;
				runval = XCF_ReadByte(f);
				runtype = 0;
			}
			else
			{	//unpacked run
				if (runsize == 128)
				{
					runsize = XCF_ReadByte(f)<<8;
					runsize |= XCF_ReadByte(f);
				}
				else
					runsize = 256-runsize;
				runval = -1;
				runtype = 1;
			}
			if (!runsize)
				return false;	//buggy input

			while(runsize --> 0)
			{
				if (runtype)
					runval = XCF_ReadByte(f);
				out[rowbyte] = runval;
				rowbyte+=pxsize;

				if (rowbyte == bytew)
				{	//reached the end of the row, move on to the next
					if (++y == h)
					{
						if (runsize)
							return false;	//runsize was too long...
						goto nextchan; //reached the end of the tile.
					}
					rowbyte = 0;
					out += bytestride;
				}
			}
		}
	}
	else
		return false;
}
struct xcf_heirachy_s
{
	quint32_t width;
	quint32_t height;
	quint32_t bpp;
	qbyte	*data;
};
static struct xcf_heirachy_s XCF_ReadHeirachy(struct xcf_s *f)
{
	struct xcf_heirachy_s ctx;
	quint32_t x, y, lw, lh;
	qofs_t ofs, tofs;
	if (!f->offset)
	{
		memset(&ctx, 0, sizeof(ctx));
		return ctx;
	}
	ctx.width = XCF_Read32(f);
	ctx.height = XCF_Read32(f);
	ctx.bpp = XCF_Read32(f);
	ctx.data = NULL;

	ofs = XCF_ReadOffset(f);
	while (XCF_ReadOffset(f))
		;	//we don't care about these dummy offsets. we could use them for mipmaps but we don't expect them to be valid or bug-free

	f->offset = ofs;	//jump to level 0
	lw = XCF_Read32(f);
	lh = XCF_Read32(f);
	if (lw == ctx.width && lh == ctx.height)
	{
		ctx.data = BZ_Malloc(ctx.width*(size_t)ctx.bpp*ctx.height);

		for (y = 0; y < ctx.height; y+=lh)
		{
			lh = min(64, ctx.height-y);
			for (x = 0; x < ctx.width; x+=lw)
			{
				lw = min(64, ctx.width-x);
				tofs = XCF_ReadOffset(f);

				ofs = f->offset;
				f->offset = tofs;
				if (!XCF_ReadTile(f, ctx.data + x*ctx.bpp + y*ctx.width*ctx.bpp, ctx.bpp, ctx.width*ctx.bpp, lw*ctx.bpp, lh))
				{
					BZ_Free(ctx.data);
					ctx.data = NULL;
					return ctx;
				}
				f->offset = ofs;
			}
		}
	}
	return ctx;
}
static struct xcf_heirachy_s XCF_ReadChannel(struct xcf_s *f)
{
	struct xcf_heirachy_s h;
	char name[1024];
	quint32_t width = XCF_Read32(f);
	quint32_t height = XCF_Read32(f);
	unsigned int proptype, propsize;
	XCF_ReadString(f, name, sizeof(name));
	for(;;)
	{
		proptype = XCF_Read32(f);
		propsize = XCF_Read32(f);
		if (!proptype)
			break;
		else if (proptype == 3) //prop_active_channel
			; //ui state
		else if (proptype == 16) //prop_colour
			f->offset += 3; //ui state
		else if (proptype == 38) //prop_colour
			f->offset += 3*4; //ui state
		else if (proptype == 4) //prop_selection
			; //ui state
		else if (proptype == 14) //prop_showmask
			f->offset += 4; //ui state
		else
		{
			Con_DPrintf("Unknown channel property %i\n", proptype);
			f->offset += propsize;
		}
	}
	f->offset = XCF_ReadOffset(f);
	h = XCF_ReadHeirachy(f);
	if (h.width != width || h.height != height)
	{
		BZ_Free(h.data);
		h.data = NULL;
	}
	return h;
}

static float XCF_BigFloat(void *in)
{
	union
	{
		qbyte b[4];
		float f;
	} u;
	u.b[0] = ((qbyte*)in)[3];
	u.b[1] = ((qbyte*)in)[2];
	u.b[2] = ((qbyte*)in)[1];
	u.b[3] = ((qbyte*)in)[0];
	return u.f;
}
static float XCF_BigShort(void *in)
{
	return (((qbyte*)in)[0]<<8)|(((qbyte*)in)[1]<<0);
}
static float XCF_HalfFloat(void *in)
{
	return HalfToFloat(XCF_BigShort(in));
}

static qboolean XCF_CombineLayer(struct xcf_s *f)
{
	struct xcf_heirachy_s h, l={0};
	unsigned int proptype, propsize;
	size_t width, height, type, heirachyoffset, layermaskoffset;
	char name[1024];
	quint32_t applylayermask = false;
	qint32_t blendmode = 0;
	qboolean unsupported = false;
	quint32_t x,y, ofsx=0,ofsy=0;
	quint32_t visible = true;
	float opacity = 1;
	width = XCF_Read32(f);
	height = XCF_Read32(f);
	type = XCF_Read32(f);
	XCF_ReadString(f, name, sizeof(name));

	for(;;)
	{
		proptype = XCF_Read32(f);
		propsize = XCF_Read32(f);
		if (!proptype)
			break;
		else if (proptype == 2) //prop_active_layer
			; //ui state
		else if (proptype == 11) //prop_apply_mask
			applylayermask = XCF_Read32(f);
		else if (proptype == 35) //prop_composite_mode
			/*compositemode =*/ XCF_Read32(f);
		else if (proptype == 36) //prop_composite_space
			/*colourspace = */ XCF_Read32(f);
		else if (proptype == 12) //prop_edit_mask
			/*editmask = */ XCF_Read32(f); //ui data, uninteresting
		else if (proptype == 5) //prop_floating_selection
		{	//just ignore this layer.
			//FIXME: the *other* layer needs to composite differently
			return true;
			XCF_ReadOffset(f);
		}
		else if (proptype == 6) //prop_opacity
			opacity = XCF_Read32(f)/255.0;
		else if (proptype == 33) //prop_float_opacity
			opacity = XCF_ReadFloat(f);
		else if (proptype == 34) //prop_colour_tag
			/*tag = */XCF_Read32(f);
		else if (proptype == 32) //prop_lock_content
			/*lockcontent = */XCF_Read32(f);
		else if (proptype == 29) //prop_group_item
		{
			Con_DPrintf("Unsupported layer property: prop_group_item\n");
			visible = false;
//			unsupported = true; //panic
		}
		else if (proptype == 30) //prop_item_path
		{
			Con_DPrintf("Unsupported layer property: prop_item_path\n");
			visible = false;
//			unsupported = true;
			while(XCF_ReadOffset(f))
				;
		}
		else if (proptype == 31) //prop_group_item_flags
			XCF_Read32(f);
		else if (proptype == 9) //prop_linked
			/*layerislinked = */ XCF_Read32(f); //ui data, uninteresting
		else if (proptype == 10) //prop_lock_alpha
			XCF_Read32(f); //ui state
		else if (proptype == 7) //prop_mode
			blendmode = XCF_Read32(f);
		else if (proptype == 8) //prop_visible
			visible = XCF_Read32(f);
		else if (proptype == 15) //prop_offsets
		{
			ofsx = XCF_Read32(f);
			ofsy = XCF_Read32(f);
		}
		else if (proptype == 13) //prop_show_mask
			XCF_Read32(f); //ui data, uninteresting
		else if (proptype == 26) //prop_text_layer_flags
			XCF_Read32(f); //ui data, uninteresting
		else if (proptype == 28) //prop_lock_content
			XCF_Read32(f); //ui data, uninteresting
		else if (proptype == 20) //prop_tattoo
			XCF_Read32(f); //ui data, uninteresting
		else if (proptype == 37) //prop_blend_space
			/*blendspace = */XCF_Read32(f);
		else
		{
			Con_DPrintf("Unknown layer(\"%s\") property %i\n", name, proptype);
			f->offset += propsize;
		}
	}

	heirachyoffset = XCF_ReadOffset(f);
	layermaskoffset = XCF_ReadOffset(f);

	if (unsupported || !visible)
		heirachyoffset = layermaskoffset = 0;

	f->offset = heirachyoffset;
	h = XCF_ReadHeirachy(f);

	if (applylayermask)
	{	//blend the alpha...?
		f->offset = layermaskoffset;
		l = XCF_ReadChannel(f);
		if (!l.data || l.bpp!=1 || l.width != width || l.height != height)
			applylayermask = false; //erk?
	}

	if (!h.data)
	{	//its valid to have just a layermask...
		h.bpp = 0; //don't try reading anything.
		h.width = width;
		h.height = height;
	}
	else if (h.width != width || h.height != height)
		unsupported = true;	//level0 must match the layer's size

	if (unsupported || !visible)
		;
	else
	{
		width = min(h.width, f->width-ofsx);
		height = min(h.height, f->height-ofsy);
		if (ofsx < f->width && ofsy < f->height)
		{
			qbyte *in=h.data;
			vec4_t px;
			float sa, da, k;
			if (f->outformat == PTI_RGBA32F)
			{
				float *out = (float*)f->flat + ofsx*4 + ofsy*f->width*4;
				for (y = 0; y < height; y++, out += 4*(f->width-width), in += h.bpp*(h.width-width))
				{
					for (x = 0; x < width; x++, out+=4, in+=h.bpp)
					{
						switch(h.bpp)
						{
						case 16: Vector4Set(px, XCF_BigFloat(in+0), XCF_BigFloat(in+4), XCF_BigFloat(in+8), XCF_BigFloat(in+12)); break;
						case 12: Vector4Set(px, XCF_BigFloat(in+0), XCF_BigFloat(in+4), XCF_BigFloat(in+8), 1); break;
						case 8:	Vector4Set(px, XCF_BigFloat(in+0), XCF_BigFloat(in+0), XCF_BigFloat(in+0), XCF_BigFloat(in+4)); break;
						case 4:	Vector4Set(px, XCF_BigFloat(in+0), XCF_BigFloat(in+0), XCF_BigFloat(in+0), 1); break;
						//other bpp are invalid
						default:
						case 0: Vector4Set(px,  1,  1,  1,  1);	break;
						}
						if (applylayermask)
							px[3] *= l.data[x+y*width]/(float)0xff; //always bytes.
						px[3] *= opacity;
						switch(blendmode)
						{
						case 0:		//normal(legacy)
						case 28:	//normal
							da = out[3];
							sa = px[3];
							k = 1-(1-da)*(1-sa);
							out[3] = k;
							k = sa/k;
							out[0] = out[0]*(1-k)+px[0]*k;
							out[1] = out[1]*(1-k)+px[1]*k;
							out[2] = out[2]*(1-k)+px[2]*k;
							break;
						default:
							Con_Printf("xcf: blend mode %i is not supported\n", blendmode);
							unsupported = true;
							goto parseerror;
						}
					}
				}
			}
			else if (f->outformat == PTI_RGBA16F)
			{
				unsigned short *out = (unsigned short*)f->flat + ofsx*4 + ofsy*f->width*4;
				for (y = 0; y < height; y++, out += 4*(f->width-width), in += h.bpp*(h.width-width))
				{
					for (x = 0; x < width; x++, out+=4, in+=h.bpp)
					{
						switch(h.bpp)
						{
						case 8: Vector4Set(px, XCF_HalfFloat(in+0), XCF_HalfFloat(in+2), XCF_HalfFloat(in+4), XCF_HalfFloat(in+6)); break;
						case 6: Vector4Set(px, XCF_HalfFloat(in+0), XCF_HalfFloat(in+2), XCF_HalfFloat(in+4), 1); break;
						case 4:	Vector4Set(px, XCF_HalfFloat(in+0), XCF_HalfFloat(in+0), XCF_HalfFloat(in+0), XCF_HalfFloat(in+2)); break;
						case 2:	Vector4Set(px, XCF_HalfFloat(in+0), XCF_HalfFloat(in+0), XCF_HalfFloat(in+0), 1); break;
						//other bpp are invalid
						default:
						case 0: Vector4Set(px,  1,  1,  1,  1);	break;
						}
						if (applylayermask)
							px[3] *= l.data[x+y*width]/(float)0xff; //always bytes.
						px[3] *= opacity;
						switch(blendmode)
						{
						case 0:		//normal(legacy)
						case 28:	//normal
							da = HalfToFloat(out[3]);
							sa = px[3];
							k = 1-(1-da)*(1-sa);
							out[3] = FloatToHalf(k);
							k = sa/k;
							out[0] = FloatToHalf(HalfToFloat(out[0])*(1-k)+px[0]*k);
							out[1] = FloatToHalf(HalfToFloat(out[1])*(1-k)+px[1]*k);
							out[2] = FloatToHalf(HalfToFloat(out[2])*(1-k)+px[2]*k);
							break;
						default:
							Con_Printf("xcf: blend mode %i is not supported\n", blendmode);
							unsupported = true;
							goto parseerror;
						}
					}
				}
			}
			else if (f->outformat == PTI_RGBA16)
			{
				unsigned short *out = (unsigned short*)f->flat + ofsx*4 + ofsy*f->width*4;
				for (y = 0; y < height; y++, out += 4*(f->width-width), in += h.bpp*(h.width-width))
				{
					for (x = 0; x < width; x++, out+=4, in+=h.bpp)
					{
						switch(h.bpp)
						{
						case 8: Vector4Set(px, XCF_BigShort(in+0), XCF_BigShort(in+2), XCF_BigShort(in+4), XCF_BigShort(in+6));	break;
						case 6: Vector4Set(px, XCF_BigShort(in+0), XCF_BigShort(in+2), XCF_BigShort(in+4),  0xffff);	break;
						case 4: Vector4Set(px, XCF_BigShort(in+0), XCF_BigShort(in+0), XCF_BigShort(in+0), XCF_BigShort(in+2));	break;
						case 2: Vector4Set(px, XCF_BigShort(in+0), XCF_BigShort(in+0), XCF_BigShort(in+0),  0xffff);	break;
						default:
						case 0: Vector4Set(px,  0xffff,  0xffff,  0xffff,  0xffff);	break;
						}
						if (applylayermask)
							px[3] *= l.data[x+y*width]/(float)0xff;
						px[3] *= opacity;
						switch(blendmode)
						{
						case 0:		//normal(legacy)
						case 28:	//normal
							da = out[3]/(float)0xffff;
							sa = px[3]/0xffff;
							k = 1-(1-da)*(1-sa);
							out[3] = k*0xffff;
							k = sa/k;
							out[0] = out[0]*(1-k)+px[0]*k;
							out[1] = out[1]*(1-k)+px[1]*k;
							out[2] = out[2]*(1-k)+px[2]*k;
							break;
						default:
							Con_Printf("xcf: blend mode %i is not supported\n", blendmode);
							unsupported = true;
							goto parseerror;
						}
					}
				}
			}
			else if (f->outformat == PTI_RGBA8)
			{
				qbyte *out = f->flat + ofsx*4 + ofsy*f->width*4;
				for (y = 0; y < height; y++, out += 4*(f->width-width), in += h.bpp*(h.width-width))
				{
					for (x = 0; x < width; x++, out+=4, in+=h.bpp)
					{
						switch(h.bpp)
						{
						case 4: Vector4Set(px, in[0], in[1], in[2], in[3]);	break;
						case 3: Vector4Set(px, in[0], in[1], in[2],  0xff);	break;
						case 2: Vector4Set(px, in[0], in[0], in[0], in[1]);	break;
						case 1: Vector4Set(px, in[0], in[0], in[0],  0xff);	break;
						default:
						case 0: Vector4Set(px,  0xff,  0xff,  0xff,  0xff);	break;
						}
						if (applylayermask)
							px[3] *= l.data[x+y*width]/(float)0xff;
						px[3] *= opacity;
						switch(blendmode)
						{
						case 0:		//normal(legacy)
						case 28:	//normal
							da = out[3]/255.0;
							sa = px[3]/255;
							k = 1-(1-da)*(1-sa);
							out[3] = k*255;
							k = sa/k;
							out[0] = out[0]*(1-k)+px[0]*k;
							out[1] = out[1]*(1-k)+px[1]*k;
							out[2] = out[2]*(1-k)+px[2]*k;
							break;
						default:
							Con_Printf("xcf: blend mode %i is not supported\n", blendmode);
							unsupported = true;
							goto parseerror;
						}
					}
				}
			}
			else
			{
				Con_Printf("xcf: colour precision %i is not supported\n", f->precision);
				unsupported = true;
				goto parseerror;
			}
		}
	}

parseerror:
	BZ_Free(l.data);
	BZ_Free(h.data);

	(void)type;
	(void)layermaskoffset;

	return !unsupported;
}
qbyte *ReadXCFFile(const qbyte *filedata, size_t len, const char *fname, int *width, int *height, uploadfmt_t *format)
{
	size_t offs;
	struct xcf_s ctx;
	unsigned int bb,bw,bh,bd;
	if (len < 14 || strncmp(filedata, "gimp xcf ", 9) || filedata[13])
		return NULL;
	memset(&ctx, 0, sizeof(ctx));
	ctx.version = atoi(filedata+10);
	ctx.filestart = filedata;
	ctx.filesize = len;
	ctx.offset = 14;

	ctx.precision = 150;
	ctx.outformat = PTI_RGBA8;
	ctx.width = XCF_Read32(&ctx);
	ctx.height = XCF_Read32(&ctx);
	ctx.basetype = XCF_Read32(&ctx);
	if (ctx.basetype != 0/*rgb*/ && ctx.basetype != 1/*grey*/)
	{
		Con_Printf("%s: xcf paletted mode is not supported\n", fname);
		return NULL; //doesn't really matter what format it is, we're going to output rgba regardless. we can just do it based upon the bytes per pixel.
	}
	if (ctx.version >= 4)
	{
		ctx.precision = XCF_Read32(&ctx);
		if (ctx.version < 7)
			ctx.precision = 150; //dev versions have different interpretations, just ignore it so that we don't have to handle that mess.
		switch(ctx.precision)
		{
		case 100:	ctx.outformat = PTI_RGBA8;	break;
		case 150:	ctx.outformat = PTI_RGBA8/*_SRGB*/;	break; //usually this one... but we don't care too much about srgb... for some reason.
		case 200:	ctx.outformat = PTI_RGBA16;	break;
		case 250:	ctx.outformat = PTI_RGBA16/*_SRGB*/;	break;
		//case 300:	ctx.outformat = PTI_RGBA32;	break;
		//case 350:	ctx.outformat = PTI_RGBA32/*_SRGB*/;	break;
		case 500:	ctx.outformat = PTI_RGBA16F;	break;
		case 550:	ctx.outformat = PTI_RGBA16F/*_SRGB*/;	break;
		case 600:	ctx.outformat = PTI_RGBA32F;	break;
		case 650:	ctx.outformat = PTI_RGBA32F/*_SRGB*/;	break;
		default:
			Con_Printf("%s: xcf colour precision is not supported\n", fname);
			return NULL;
		}
	}
	if (!format && ctx.outformat != PTI_RGBA8)
		return NULL;	//caller insists on rgba8 :(
	XCF_ReadHeaderProperties(&ctx);
	while((offs=XCF_ReadOffset(&ctx)))
	{
		ctx.layeroffsets = realloc(ctx.layeroffsets, sizeof(*ctx.layeroffsets)*(ctx.numlayers+1));
		ctx.layeroffsets[ctx.numlayers++] = offs;
	}
	//channels

	//without any layers, its fully transparent
	Image_BlockSizeForEncoding(ctx.outformat, &bb,&bw,&bh,&bd); //just for the bb...
	ctx.flat = Z_Malloc(ctx.width*ctx.height*bb);
	if (format)
		*format = ctx.outformat;
	*width = ctx.width;
	*height = ctx.height;

	while(ctx.numlayers --> 0)
	{
		ctx.offset = ctx.layeroffsets[ctx.numlayers];
		if (!XCF_CombineLayer(&ctx))
		{
			Z_Free(ctx.flat);
			ctx.flat = NULL;
			break;
		}
	}


	BZ_Free(ctx.layeroffsets);
	return ctx.flat;
}

#endif // IMAGEFMT_XCF
