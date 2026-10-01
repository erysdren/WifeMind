#include "image.h"

#ifdef IMAGEFMT_BLP

struct pendingtextureinfo *Image_ReadBLPFile(unsigned int flags, const char *fname, qbyte *filedata, size_t filesize)
{
	//FIXME: cba with endian.
	int miplevel;
	int w, h, i;
	struct blp_s
	{
		char blp2[4];
		int type;
		qbyte encoding;
		qbyte alphadepth;
		qbyte alphaencoding;
		qbyte hasmips;
		unsigned int xres;
		unsigned int yres;
		unsigned int mipoffset[16];
		unsigned int mipsize[16];
		unsigned int palette[256];
	} *blp;
	unsigned int *tmpmem = NULL;
	unsigned char *in;
	unsigned int inlen;

	struct pendingtextureinfo *mips;

	blp = (void*)filedata;

	if (memcmp(blp->blp2, "BLP2", 4) || blp->type != 1)
		return NULL;

	mips = Z_Malloc(sizeof(*mips));
	mips->mipcount = 0;
	mips->type = PTI_2D;

	w = LittleLong(blp->xres);
	h = LittleLong(blp->yres);

	if (blp->encoding == 2)
	{
		//s3tc/dxt
		switch(blp->alphaencoding)
		{
		default:
		case 0: //dxt1
			if (blp->alphadepth)
				mips->encoding = PTI_BC1_RGBA;
			else
				mips->encoding = PTI_BC1_RGB;
			break;
		case 1: //dxt2/3
			mips->encoding = PTI_BC2_RGBA;
			break;
		case 7: //dxt4/5
			mips->encoding = PTI_BC3_RGBA;
			break;
		}
		for (miplevel = 0; miplevel < 16; )
		{
			if (!w && !h)	//shrunk to no size
				break;
			if (!w)
				w = 1;
			if (!h)
				h = 1;
			if (!blp->mipoffset[miplevel] || !blp->mipsize[miplevel] || blp->mipoffset[miplevel]+blp->mipsize[miplevel] > filesize)	//no data
				break;
			mips->mip[miplevel].width = w;
			mips->mip[miplevel].height = h;
			mips->mip[miplevel].depth = 1;
			mips->mip[miplevel].data = filedata + LittleLong(blp->mipoffset[miplevel]);
			mips->mip[miplevel].datasize = LittleLong(blp->mipsize[miplevel]);

			miplevel++;
			if (!blp->hasmips || (flags & IF_NOMIPMAP))
				break;
			w >>= 1;
			h >>= 1;
		}
		mips->mipcount = miplevel;
		mips->extrafree = filedata;
	}
	else
	{
		mips->encoding = PTI_BGRA8;
		for (miplevel = 0; miplevel < 16; )
		{
			if (!w && !h)
				break;
			if (!w)
				w = 1;
			if (!h)
				h = 1;
			//if we ran out of mips to load, give up.
			if (!blp->mipoffset[miplevel] || !blp->mipsize[miplevel] || blp->mipoffset[miplevel]+blp->mipsize[miplevel] > filesize)
			{
				//if we got at least one mip, cap the mips. might help save some ram? naaah...
				//if this is the first mip, well, its completely fucked.
				break;
			}

			in = filedata + LittleLong(blp->mipoffset[miplevel]);
			inlen = LittleLong(blp->mipsize[miplevel]);

			if (inlen != w*h+((w*h*blp->alphadepth+7)>>3))
			{
				Con_Printf("%s: mip level %i does not contain the correct amount of data\n", fname, miplevel);
				break;
			}

			mips->mip[miplevel].width = w;
			mips->mip[miplevel].height = h;
			mips->mip[miplevel].depth = 1;
			mips->mip[miplevel].datasize = 4*w*h;
			mips->mip[miplevel].data = tmpmem = BZ_Malloc(4*w*h);
			mips->mip[miplevel].needfree = true;

			//load the rgb data first (8-bit paletted)
			for (i = 0; i < w*h; i++)
				tmpmem[i] = blp->palette[*in++] | 0xff000000;

			//and then change the alpha bits accordingly.
			switch(blp->alphadepth)
			{
			case 0:
				//BGRX palette, 8bit
				break;
			case 1:
				//BGRX palette, 8bit
				//1bit trailing alpha
				for (i = 0; i < w*h; i+=8, in++)
				{
					tmpmem[i+0] = (tmpmem[i+0] & 0xffffff) | ((*in&0x01)?0xff000000:0);
					tmpmem[i+1] = (tmpmem[i+1] & 0xffffff) | ((*in&0x02)?0xff000000:0);
					tmpmem[i+2] = (tmpmem[i+2] & 0xffffff) | ((*in&0x04)?0xff000000:0);
					tmpmem[i+3] = (tmpmem[i+3] & 0xffffff) | ((*in&0x08)?0xff000000:0);
					tmpmem[i+4] = (tmpmem[i+4] & 0xffffff) | ((*in&0x10)?0xff000000:0);
					tmpmem[i+5] = (tmpmem[i+5] & 0xffffff) | ((*in&0x20)?0xff000000:0);
					tmpmem[i+6] = (tmpmem[i+6] & 0xffffff) | ((*in&0x40)?0xff000000:0);
					tmpmem[i+7] = (tmpmem[i+7] & 0xffffff) | ((*in&0x80)?0xff000000:0);
				}
				break;
			case 4:
				//BGRX palette, 8bit
				//4bit trailing alpha
				for (i = 0; i < w*h; i++)
					tmpmem[i] = (tmpmem[i] & 0xffffff) | (*in++*0x11000000);
				break;
			case 8:
				//BGRX palette, 8bit
				//8bit trailing alpha
				for (i = 0; i < w*h; i++)
					tmpmem[i] = (tmpmem[i] & 0xffffff) | (*in++<<24);
				break;
			}

			miplevel++;
			if (!blp->hasmips || (flags & IF_NOMIPMAP))
				break;
			w = w>>1;
			h = h>>1;
		}
		BZ_Free(filedata);
		mips->mipcount = miplevel;
	}
	return mips;
}

#endif // IMAGEFMT_BLP
