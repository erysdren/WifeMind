#include "image.h"

#ifdef IMAGEFMT_DDS

typedef struct {
	unsigned int dwSize;
	unsigned int dwFlags;
	unsigned int dwFourCC;

	unsigned int bitcount;
	unsigned int redmask;
	unsigned int greenmask;
	unsigned int bluemask;
	unsigned int alphamask;
} ddspixelformat_t;

typedef struct {
	unsigned int dwSize;
	unsigned int dwFlags;
	unsigned int dwHeight;
	unsigned int dwWidth;
	unsigned int dwPitchOrLinearSize;
	unsigned int dwDepth;
	unsigned int dwMipMapCount;
	unsigned int dwReserved1[11];
	ddspixelformat_t ddpfPixelFormat;
	unsigned int ddsCaps[4];
	unsigned int dwReserved2;
} ddsheader_t;
typedef struct {
	unsigned int dxgiformat;
	unsigned int resourcetype; //0=unknown, 1=buffer, 2=1d, 3=2d, 4=3d
	unsigned int miscflag;	//singular... yeah. 4=cubemap.
	unsigned int arraysize;
	unsigned int miscflags2;
} dds10header_t;

struct pendingtextureinfo *Image_ReadDDSFile(unsigned int flags, const char *fname, qbyte *filedata, size_t filesize)
{
	int nummips;
	int mipnum;
	int datasize;
	unsigned int w, h, d;
	unsigned int blockwidth, blockheight, blockdepth, blockbytes;
	struct pendingtextureinfo *mips;
	int encoding;
	int layers = 1, layer;
	int ttype;

	ddsheader_t fmtheader;
	dds10header_t fmt10header;
	qbyte *fileend = filedata + filesize;

	if (filesize < sizeof(fmtheader) || *(int*)filedata != (('D'<<0)|('D'<<8)|('S'<<16)|(' '<<24)))
		return NULL;

	memcpy(&fmtheader, filedata+4, sizeof(fmtheader));
	if (fmtheader.dwSize != sizeof(fmtheader))
		return NULL;	//corrupt/different version
	fmtheader.dwSize += 4;
	memset(&fmt10header, 0, sizeof(fmt10header));

	fmt10header.arraysize = (fmtheader.ddsCaps[1] & 0x200)?6:1; //cubemaps need 6 faces...

	nummips = fmtheader.dwMipMapCount;
	if (nummips < 1)
		nummips = 1;
	if (nummips > countof(mips->mip))
		return NULL;

	if (!(fmtheader.ddpfPixelFormat.dwFlags & 4))
	{
#define IsPacked(bits,r,g,b,a)	fmtheader.ddpfPixelFormat.bitcount==bits&&fmtheader.ddpfPixelFormat.redmask==r&&fmtheader.ddpfPixelFormat.greenmask==g&&fmtheader.ddpfPixelFormat.bluemask==b&&fmtheader.ddpfPixelFormat.alphamask==a
		if (IsPacked(24, 0xff0000, 0x00ff00, 0x0000ff, 0))
			encoding = PTI_BGR8;
		else if (IsPacked(24, 0x000000, 0x00ff00, 0xff0000, 0))
			encoding = PTI_RGB8;
		else if (IsPacked(32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000))
			encoding = PTI_BGRA8;
		else if (IsPacked(32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
			encoding = PTI_RGBA8;
		else if (IsPacked(32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0))
			encoding = PTI_BGRX8;
		else if (IsPacked(32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0))
			encoding = PTI_RGBX8;
		else if (IsPacked(32, 0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000))
			encoding = PTI_A2BGR10;
		else if (IsPacked(16, 0xf800, 0x07e0, 0x001f, 0))
			encoding = PTI_RGB565;
		else if (IsPacked(16, 0xf800, 0x07c0, 0x003e, 0x0001))
			encoding = PTI_RGBA5551;
		else if (IsPacked(16, 0x7c00, 0x03e0, 0x001f, 0x8000))
			encoding = PTI_ARGB1555;
		else if (IsPacked(16, 0xf000, 0x0f00, 0x00f0, 0x000f))
			encoding = PTI_RGBA4444;
		else if (IsPacked(16, 0x0f00, 0x00f0, 0x000f, 0xf000))
			encoding = PTI_ARGB4444;
		else if (IsPacked( 8, 0x000000ff, 0x00000000, 0x00000000, 0x00000000))
			encoding = (fmtheader.ddpfPixelFormat.dwFlags&0x20000)?PTI_L8:PTI_R8;
		else if (IsPacked(16, 0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
			encoding = PTI_L8A8;
		else
		{
			Con_Printf("Unsupported non-fourcc dds in %s\n", fname);
			Con_Printf(" bits: %u\n", fmtheader.ddpfPixelFormat.bitcount);
			Con_Printf("  red: %08x\n", fmtheader.ddpfPixelFormat.redmask);
			Con_Printf("green: %08x\n", fmtheader.ddpfPixelFormat.greenmask);
			Con_Printf(" blue: %08x\n", fmtheader.ddpfPixelFormat.bluemask);
			Con_Printf("alpha: %08x\n", fmtheader.ddpfPixelFormat.alphamask);
			Con_Printf(" used: %08x\n", fmtheader.ddpfPixelFormat.redmask^fmtheader.ddpfPixelFormat.greenmask^fmtheader.ddpfPixelFormat.bluemask^fmtheader.ddpfPixelFormat.alphamask);
			return NULL;
		}
#undef IsPacked
	}
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('D'<<0)|('X'<<8)|('T'<<16)|('1'<<24)))
		encoding = PTI_BC1_RGBA;	//alpha or not? Assume yes, and let the drivers decide.
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('D'<<0)|('X'<<8)|('T'<<16)|('2'<<24)))	//dx3 with premultiplied alpha
	{
//		if (!(tex->flags & IF_PREMULTIPLYALPHA))
//			return false;
		encoding = PTI_BC2_RGBA;
	}
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('D'<<0)|('X'<<8)|('T'<<16)|('3'<<24)))
	{
//		if (tex->flags & IF_PREMULTIPLYALPHA)
//			return false;
		encoding = PTI_BC2_RGBA;
	}
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('D'<<0)|('X'<<8)|('T'<<16)|('4'<<24)))	//dx5 with premultiplied alpha
	{
//		if (!(tex->flags & IF_PREMULTIPLYALPHA))
//			return false;
		encoding = PTI_BC3_RGBA;
	}
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('D'<<0)|('X'<<8)|('T'<<16)|('5'<<24)))
	{
//		if (tex->flags & IF_PREMULTIPLYALPHA)
//			return false;
		encoding = PTI_BC3_RGBA;
	}
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('A'<<0)|('T'<<8)|('I'<<16)|('1'<<24))
		||   *(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('B'<<0)|('C'<<8)|('4'<<16)|('U'<<24)))
		encoding = PTI_BC4_R;
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('A'<<0)|('T'<<8)|('I'<<16)|('2'<<24))
		||   *(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('B'<<0)|('C'<<8)|('5'<<16)|('U'<<24)))
		encoding = PTI_BC5_RG;
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('B'<<0)|('C'<<8)|('4'<<16)|('S'<<24)))
		encoding = PTI_BC4_R_SNORM;
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('B'<<0)|('C'<<8)|('5'<<16)|('S'<<24)))
		encoding = PTI_BC5_RG_SNORM;
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('E'<<0)|('T'<<8)|('C'<<16)|('2'<<24)))
		encoding = PTI_ETC2_RGB8;
	else if (*(int*)&fmtheader.ddpfPixelFormat.dwFourCC == (('D'<<0)|('X'<<8)|('1'<<16)|('0'<<24)))
	{
		//this has some weird extra header with dxgi format types.
		memcpy(&fmt10header, filedata+fmtheader.dwSize, sizeof(fmt10header));
		fmtheader.dwSize += sizeof(fmt10header);
		switch(fmt10header.dxgiformat)
		{
		case 0x0/*DXGI_FORMAT_UNKNOWN*/:				encoding = PTI_INVALID;			break;
		case 0x1/*DXGI_FORMAT_R32G32B32A32_TYPELESS*/:	encoding = PTI_INVALID;			break;
		case 0x2/*DXGI_FORMAT_R32G32B32A32_FLOAT*/:		encoding = PTI_RGBA32F;			break;
//		case 0x3/*DXGI_FORMAT_R32G32B32A32_UINT*/:		encoding = PTI_INVALID;			break;
//		case 0x4/*DXGI_FORMAT_R32G32B32A32_SINT*/:		encoding = PTI_INVALID;			break;
//		case 0x5/*DXGI_FORMAT_R32G32B32_TYPELESS*/:		encoding = PTI_INVALID;			break;
		case 0x6/*DXGI_FORMAT_R32G32B32_FLOAT*/:		encoding = PTI_RGB32F;			break;
//		case 0x7/*DXGI_FORMAT_R32G32B32_UINT*/:			encoding = PTI_INVALID;			break;
//		case 0x8/*DXGI_FORMAT_R32G32B32_SINT*/:			encoding = PTI_INVALID;			break;
//		case 0x9/*DXGI_FORMAT_R16G16B16A16_TYPELESS*/:	encoding = PTI_INVALID;			break;
		case 0xa/*DXGI_FORMAT_R16G16B16A16_FLOAT*/:		encoding = PTI_RGBA16F;			break;
		case 0xb/*DXGI_FORMAT_R16G16B16A16_UNORM*/:		encoding = PTI_RGBA16;			break;
//		case 0xc/*DXGI_FORMAT_R16G16B16A16_UINT*/:		encoding = PTI_INVALID;			break;
//		case 0xd/*DXGI_FORMAT_R16G16B16A16_SNORM*/:		encoding = PTI_INVALID;			break;
//		case 0xe/*DXGI_FORMAT_R16G16B16A16_SINT*/:		encoding = PTI_INVALID;			break;
//		case 0xf/*DXGI_FORMAT_R32G32_TYPELESS*/:		encoding = PTI_INVALID;			break;
//		case 0x10/*DXGI_FORMAT_R32G32_FLOAT*/:			encoding = PTI_INVALID;			break;
//		case 0x11/*DXGI_FORMAT_R32G32_UINT*/:			encoding = PTI_INVALID;			break;
//		case 0x12/*DXGI_FORMAT_R32G32_SINT*/:			encoding = PTI_INVALID;			break;
//		case 0x13/*DXGI_FORMAT_R32G8X24_TYPELESS*/:		encoding = PTI_INVALID;			break;
//		case 0x14/*DXGI_FORMAT_D32_FLOAT_S8X24_UINT*/:	encoding = PTI_DEPTH32_8;		break;
//		case 0x15/*DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS*/:encoding = PTI_INVALID;		break;
//		case 0x16/*DXGI_FORMAT_X32_TYPELESS_G8X24_UINT*/:encoding = PTI_INVALID;		break;
//		case 0x17/*DXGI_FORMAT_R10G10B10A2_TYPELESS*/:	encoding = PTI_INVALID;			break;
		case 0x18/*DXGI_FORMAT_R10G10B10A2_UNORM*/:		encoding = PTI_A2BGR10;			break;
//		case 0x19/*DXGI_FORMAT_R10G10B10A2_UINT*/:		encoding = PTI_INVALID;			break;
		case 0x1a/*DXGI_FORMAT_R11G11B10_FLOAT*/:		encoding = PTI_B10G11R11F;		break;
//		case 0x1b/*DXGI_FORMAT_R8G8B8A8_TYPELESS*/:		encoding = PTI_INVALID;			break;
		case 0x1c/*DXGI_FORMAT_R8G8B8A8_UNORM*/:		encoding = PTI_RGBA8;			break;
		case 0x1d/*DXGI_FORMAT_R8G8B8A8_UNORM_SRGB*/:	encoding = PTI_RGBA8_SRGB;		break;
//		case 0x1e/*DXGI_FORMAT_R8G8B8A8_UINT*/:			encoding = PTI_INVALID;			break;
//		case 0x1f/*DXGI_FORMAT_R8G8B8A8_SNORM*/:		encoding = PTI_INVALID;			break;
//		case 0x20/*DXGI_FORMAT_R8G8B8A8_SINT*/:			encoding = PTI_INVALID;			break;
//		case 0x21/*DXGI_FORMAT_R16G16_TYPELESS*/:		encoding = PTI_INVALID;			break;
//		case 0x22/*DXGI_FORMAT_R16G16_FLOAT*/:			encoding = PTI_INVALID;			break;
//		case 0x23/*DXGI_FORMAT_R16G16_UNORM*/:			encoding = PTI_INVALID;			break;
//		case 0x24/*DXGI_FORMAT_R16G16_UINT*/:			encoding = PTI_INVALID;			break;
//		case 0x25/*DXGI_FORMAT_R16G16_SNORM*/:			encoding = PTI_INVALID;			break;
//		case 0x26/*DXGI_FORMAT_R16G16_SINT*/:			encoding = PTI_INVALID;			break;
//		case 0x27/*DXGI_FORMAT_R32_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x28/*DXGI_FORMAT_D32_FLOAT*/:				encoding = PTI_DEPTH32;			break;
		case 0x29/*DXGI_FORMAT_R32_FLOAT*/:				encoding = PTI_R32F;			break;
//		case 0x2a/*DXGI_FORMAT_R32_UINT*/:				encoding = PTI_INVALID;			break;
//		case 0x2b/*DXGI_FORMAT_R32_SINT*/:				encoding = PTI_INVALID;			break;
//		case 0x2c/*DXGI_FORMAT_R24G8_TYPELESS*/:		encoding = PTI_INVALID;			break;
//		case 0x2d/*DXGI_FORMAT_D24_UNORM_S8_UINT*/:		encoding = PTI_DEPTH24_8;		break;
//		case 0x2e/*DXGI_FORMAT_R24_UNORM_X8_TYPELESS*/:	encoding = PTI_INVALID;			break;
//		case 0x2f/*DXGI_FORMAT_X24_TYPELESS_G8_UINT*/:	encoding = PTI_INVALID;			break;
//		case 0x30/*DXGI_FORMAT_R8G8_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x31/*DXGI_FORMAT_R8G8_UNORM*/:			encoding = PTI_RG8;				break;
//		case 0x32/*DXGI_FORMAT_R8G8_UINT*/:				encoding = PTI_INVALID;			break;
		case 0x33/*DXGI_FORMAT_R8G8_SNORM*/:			encoding = PTI_RG8_SNORM;		break;
//		case 0x34/*DXGI_FORMAT_R8G8_SINT*/:				encoding = PTI_INVALID;			break;
//		case 0x35/*DXGI_FORMAT_R16_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x36/*DXGI_FORMAT_R16_FLOAT*/:				encoding = PTI_R16F;			break;
		case 0x37/*DXGI_FORMAT_D16_UNORM*/:				encoding = PTI_DEPTH16;			break;
		case 0x38/*DXGI_FORMAT_R16_UNORM*/:				encoding = PTI_R16;				break;
//		case 0x39/*DXGI_FORMAT_R16_UINT*/:				encoding = PTI_INVALID;			break;
//		case 0x3a/*DXGI_FORMAT_R16_SNORM*/:				encoding = PTI_INVALID;			break;
//		case 0x3b/*DXGI_FORMAT_R16_SINT*/:				encoding = PTI_INVALID;			break;
//		case 0x3c/*DXGI_FORMAT_R8_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x3d/*DXGI_FORMAT_R8_UNORM*/:				encoding = PTI_R8;				break;
//		case 0x3e/*DXGI_FORMAT_R8_UINT*/:				encoding = PTI_INVALID;			break;
		case 0x3f/*DXGI_FORMAT_R8_SNORM*/:				encoding = PTI_R8_SNORM;		break;
//		case 0x40/*DXGI_FORMAT_R8_SINT*/:				encoding = PTI_INVALID;			break;
//		case 0x41/*DXGI_FORMAT_A8_UNORM*/:				encoding = PTI_A8;				break;
//		case 0x42/*DXGI_FORMAT_R1_UNORM*/:				encoding = PTI_INVALID;			break;
		case 0x43/*DXGI_FORMAT_R9G9B9E5_SHAREDEXP*/:	encoding = PTI_E5BGR9;			break;
//		case 0x44/*DXGI_FORMAT_R8G8_B8G8_UNORM*/:		encoding = PTI_INVALID;			break;
//		case 0x45/*DXGI_FORMAT_G8R8_G8B8_UNORM*/:		encoding = PTI_INVALID;			break;
//		case 0x46/*DXGI_FORMAT_BC1_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x47/*DXGI_FORMAT_BC1_UNORM*/:				encoding = PTI_BC1_RGBA;		break;
		case 0x48/*DXGI_FORMAT_BC1_UNORM_SRGB*/:		encoding = PTI_BC1_RGBA_SRGB;	break;
//		case 0x49/*DXGI_FORMAT_BC2_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x4a/*DXGI_FORMAT_BC2_UNORM*/:				encoding = PTI_BC2_RGBA;		break;
		case 0x4b/*DXGI_FORMAT_BC2_UNORM_SRGB*/:		encoding = PTI_BC2_RGBA_SRGB;	break;
//		case 0x4c/*DXGI_FORMAT_BC3_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x4d/*DXGI_FORMAT_BC3_UNORM*/:				encoding = PTI_BC3_RGBA;		break;
		case 0x4e/*DXGI_FORMAT_BC3_UNORM_SRGB*/:		encoding = PTI_BC3_RGBA_SRGB;	break;
//		case 0x4f/*DXGI_FORMAT_BC4_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x50/*DXGI_FORMAT_BC4_UNORM*/:				encoding = PTI_BC4_R;			break;
		case 0x51/*DXGI_FORMAT_BC4_SNORM*/:				encoding = PTI_BC4_R_SNORM;		break;
//		case 0x52/*DXGI_FORMAT_BC5_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x53/*DXGI_FORMAT_BC5_UNORM*/:				encoding = PTI_BC5_RG;			break;
		case 0x54/*DXGI_FORMAT_BC5_SNORM*/:				encoding = PTI_BC5_RG_SNORM;	break;
		case 0x55/*DXGI_FORMAT_B5G6R5_UNORM*/:			encoding = PTI_RGB565;			break;
		case 0x56/*DXGI_FORMAT_B5G5R5A1_UNORM*/:		encoding = PTI_ARGB1555;		break;
		case 0x57/*DXGI_FORMAT_B8G8R8A8_UNORM*/:		encoding = PTI_BGRA8;			break;
		case 0x58/*DXGI_FORMAT_B8G8R8X8_UNORM*/:		encoding = PTI_BGRX8;			break;
//		case 0x59/*DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM*/:encoding = PTI_INVALID;		break;
//		case 0x5a/*DXGI_FORMAT_B8G8R8A8_TYPELESS*/:		encoding = PTI_INVALID;			break;
		case 0x5b/*DXGI_FORMAT_B8G8R8A8_UNORM_SRGB*/:	encoding = PTI_BGRA8_SRGB;		break;
//		case 0x5c/*DXGI_FORMAT_B8G8R8X8_TYPELESS*/:		encoding = PTI_INVALID;			break;
		case 0x5d/*DXGI_FORMAT_B8G8R8X8_UNORM_SRGB*/:	encoding = PTI_BGRX8_SRGB;		break;
//		case 0x5e/*DXGI_FORMAT_BC6H_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x5f/*DXGI_FORMAT_BC6H_UF16*/:				encoding = PTI_BC6_RGB_UFLOAT;	break;
		case 0x60/*DXGI_FORMAT_BC6H_SF16*/:				encoding = PTI_BC6_RGB_SFLOAT;	break;
//		case 0x61/*DXGI_FORMAT_BC7_TYPELESS*/:			encoding = PTI_INVALID;			break;
		case 0x62/*DXGI_FORMAT_BC7_UNORM*/:				encoding = PTI_BC7_RGBA;		break;
		case 0x63/*DXGI_FORMAT_BC7_UNORM_SRGB*/:		encoding = PTI_BC7_RGBA_SRGB;	break;
//		case 0x64/*DXGI_FORMAT_AYUV*/:					encoding = PTI_INVALID;			break;
//		case 0x65/*DXGI_FORMAT_Y410*/:					encoding = PTI_INVALID;			break;
//		case 0x66/*DXGI_FORMAT_Y416*/:					encoding = PTI_INVALID;			break;
//		case 0x67/*DXGI_FORMAT_NV12*/:					encoding = PTI_INVALID;			break;
//		case 0x68/*DXGI_FORMAT_P010*/:					encoding = PTI_INVALID;			break;
//		case 0x69/*DXGI_FORMAT_P016*/:					encoding = PTI_INVALID;			break;
//		case 0x6a/*DXGI_FORMAT_420_OPAQUE*/:			encoding = PTI_INVALID;			break;
//		case 0x6b/*DXGI_FORMAT_YUY2*/:					encoding = PTI_INVALID;			break;
//		case 0x6c/*DXGI_FORMAT_Y210*/:					encoding = PTI_INVALID;			break;
//		case 0x6d/*DXGI_FORMAT_Y216*/:					encoding = PTI_INVALID;			break;
//		case 0x6e/*DXGI_FORMAT_NV11*/:					encoding = PTI_INVALID;			break;
//		case 0x6f/*DXGI_FORMAT_AI44*/:					encoding = PTI_INVALID;			break;
//		case 0x70/*DXGI_FORMAT_IA44*/:					encoding = PTI_INVALID;			break;
//		case 0x71/*DXGI_FORMAT_P8*/:					encoding = PTI_INVALID;			break;
//		case 0x72/*DXGI_FORMAT_A8P8*/:					encoding = PTI_INVALID;			break;
		case 0x73/*DXGI_FORMAT_B4G4R4A4_UNORM*/:		encoding = PTI_ARGB4444;		break;
//		case 0x82/*DXGI_FORMAT_P208*/:					encoding = PTI_INVALID;			break;
//		case 0x83/*DXGI_FORMAT_V208*/:					encoding = PTI_INVALID;			break;
//		case 0x84/*DXGI_FORMAT_V408*/:					encoding = PTI_INVALID;			break;
		case 134:	encoding = PTI_ASTC_4X4_LDR;	break;
		case 135:	encoding = PTI_ASTC_4X4_SRGB;	break;
		case 138:	encoding = PTI_ASTC_5X4_LDR;	break;
		case 139:	encoding = PTI_ASTC_5X4_SRGB;	break;
		case 142:	encoding = PTI_ASTC_5X5_LDR;	break;
		case 143:	encoding = PTI_ASTC_5X5_SRGB;	break;
		case 146:	encoding = PTI_ASTC_6X5_LDR;	break;
		case 147:	encoding = PTI_ASTC_6X5_SRGB;	break;
		case 150:	encoding = PTI_ASTC_6X6_LDR;	break;
		case 151:	encoding = PTI_ASTC_6X6_SRGB;	break;
		case 154:	encoding = PTI_ASTC_8X5_LDR;	break;
		case 155:	encoding = PTI_ASTC_8X5_SRGB;	break;
		case 158:	encoding = PTI_ASTC_8X6_LDR;	break;
		case 159:	encoding = PTI_ASTC_8X6_SRGB;	break;
		case 162:	encoding = PTI_ASTC_8X8_LDR;	break;
		case 163:	encoding = PTI_ASTC_8X8_SRGB;	break;
		case 166:	encoding = PTI_ASTC_10X5_LDR;	break;
		case 167:	encoding = PTI_ASTC_10X5_SRGB;	break;
		case 170:	encoding = PTI_ASTC_10X6_LDR;	break;
		case 171:	encoding = PTI_ASTC_10X6_SRGB;	break;
		case 174:	encoding = PTI_ASTC_10X8_LDR;	break;
		case 175:	encoding = PTI_ASTC_10X8_SRGB;	break;
		case 178:	encoding = PTI_ASTC_10X10_LDR;	break;
		case 179:	encoding = PTI_ASTC_10X10_SRGB;	break;
		case 182:	encoding = PTI_ASTC_12X10_LDR;	break;
		case 183:	encoding = PTI_ASTC_12X10_SRGB;	break;
		case 186:	encoding = PTI_ASTC_12X12_LDR;	break;
		case 187:	encoding = PTI_ASTC_12X12_SRGB;	break;

		default:
			Con_Printf("Unsupported dds10 dxgi in %s - %u\n", fname, fmt10header.dxgiformat);
			return NULL;
		}
	}
	else
	{
		Con_Printf("Unsupported dds fourcc in %s - \"%c%c%c%c\"\n", fname,
			((char*)&fmtheader.ddpfPixelFormat.dwFourCC)[0],
			((char*)&fmtheader.ddpfPixelFormat.dwFourCC)[1],
			((char*)&fmtheader.ddpfPixelFormat.dwFourCC)[2],
			((char*)&fmtheader.ddpfPixelFormat.dwFourCC)[3]);
		return NULL;
	}

	if ((fmtheader.ddsCaps[1] & 0x200) && (fmtheader.ddsCaps[1] & 0xfc00) != 0xfc00)
		return NULL;	//cubemap without all 6 faces defined.

	Image_BlockSizeForEncoding(encoding, &blockbytes, &blockwidth, &blockheight, &blockdepth);
	if (!blockbytes)
		return NULL;	//werid/unsupported

	if (fmtheader.dwFlags & 8)
	{	//explicit pitch flag. we don't support any padding, so this check exists just to be sure none is required.
		w = max(1, fmtheader.dwWidth);
		if (fmtheader.dwPitchOrLinearSize != blockbytes*(w+blockwidth-1)/blockwidth)
			return NULL;
	}
	if (fmtheader.dwFlags & 0x80000)
	{	//linear size flag. we don't support any padding, so this check exists just to be sure none is required.
		//linear-size of the top-level mip.
		size_t linearsize;
		w = max(1, fmtheader.dwWidth);
		h = max(1, fmtheader.dwHeight);
		d = max(1, fmtheader.dwDepth);
		linearsize = ((w+blockwidth-1)/blockwidth)*
							((h+blockheight-1)/blockheight)*
							((d+blockdepth-1)/blockdepth)*
							blockbytes;
		if (fmtheader.dwPitchOrLinearSize != linearsize)
			return NULL;
	}

	if (fmtheader.ddsCaps[1] & 0x200)
	{
		if (fmt10header.arraysize % 6)	//weird number of faces.
			return NULL;

		if (fmt10header.arraysize == 6)
		{
			ttype = PTI_CUBE;
			layers = 6;
		}
		else
		{
			ttype = PTI_CUBE_ARRAY;
			layers = fmt10header.arraysize;
		}
	}
	else if (fmtheader.ddsCaps[1] & 0x200000)
	{
		if (fmt10header.arraysize != 1)	//no 2d arrays
			return NULL;
		ttype = PTI_3D;
	}
	else
	{
		if (fmt10header.arraysize == 1)
			ttype = PTI_2D;
		else
			ttype = PTI_2D_ARRAY;
		layers = fmt10header.arraysize;
	}

	mips = Z_Malloc(sizeof(*mips));
	mips->mipcount = 0;
	mips->type = ttype;
	mips->extrafree = filedata;
	mips->encoding = encoding;

	filedata += fmtheader.dwSize;

	w = max(1, fmtheader.dwWidth);
	h = max(1, fmtheader.dwHeight);
	d = max(1, fmtheader.dwDepth);

	if (layers == 1)
	{	//can just use the data without copying.
		for (mipnum = 0; mipnum < nummips; mipnum++)
		{
			datasize = ((w+blockwidth-1)/blockwidth) * ((h+blockheight-1)/blockheight) * ((d+blockdepth-1)/blockdepth) * blockbytes;

			mips->mip[mipnum].data = filedata;
			mips->mip[mipnum].datasize = datasize;
			mips->mip[mipnum].width = w;
			mips->mip[mipnum].height = h;
			mips->mip[mipnum].depth = d;
			filedata += datasize;

			w = max(1, w>>1);
			h = max(1, h>>1);
			d = max(1, d>>1);
		}
		mips->mipcount = mipnum;

		if (filedata > fileend)
		{	//overflow... corrupt dds?
			Z_Free(mips);
			return NULL;
		}
	}
	else
	{	//we need to copy stuff in order to pack it properly. :(
		//allocate space and calc mip sizes
		for (mipnum = 0; mipnum < nummips; mipnum++)
		{
			datasize = ((w+blockwidth-1)/blockwidth) * ((h+blockheight-1)/blockheight) * (layers*((d+blockdepth-1)/blockdepth)) * blockbytes;
			mips->mip[mipnum].data = BZ_Malloc(datasize);
			mips->mip[mipnum].datasize = datasize;
			mips->mip[mipnum].width = w;
			mips->mip[mipnum].height = h;
			mips->mip[mipnum].depth = layers*d;

			w = max(1, w>>1);
			h = max(1, h>>1);
			d = max(1, d>>1);
		}
		mips->mipcount = mipnum;
		//and now copy over the data
		for (layer = 0; layer < layers; layer++)
		{
			for (mipnum = 0; mipnum < nummips; mipnum++)
			{
				datasize = mips->mip[mipnum].datasize/layers;
				if (filedata+datasize > fileend)
				{	//overflow... corrupt dds?
					for (mipnum = 0; mipnum < nummips; mipnum++)
						Z_Free(mips->mip[mipnum].data);
					Z_Free(mips);
					return NULL;
				}
				memcpy((qbyte*)mips->mip[mipnum].data+datasize*layer, filedata, datasize);
				filedata += datasize;
			}
		}
		//and now we're done with the source file. we might as well free it early.
		BZ_Free(mips->extrafree);
		mips->extrafree = NULL;
	}

	return mips;
}

qboolean Image_WriteDDSFile(const char *filename, enum fs_relative fsroot, struct pendingtextureinfo *mips)
{
	vfsfile_t *file;
	size_t mipnum;
	size_t a;
	dds10header_t h10={0};
	ddsheader_t h9={0};
	int *endian;

	unsigned int blockbytes, blockwidth, blockheight, blockdepth;
	unsigned int arraysize;

	Image_BlockSizeForEncoding(mips->encoding, &blockbytes, &blockwidth, &blockheight, &blockdepth);

	h9.dwSize = sizeof(h9);
	h9.ddpfPixelFormat.dwSize = sizeof(h9.ddpfPixelFormat);
	h9.dwFlags = 0;
	h9.dwFlags |= 1;			//CAPS
	h9.dwFlags |= 2;			//HEIGHT
	h9.dwFlags |= 4;			//WIDTH
	h9.dwFlags |= 0x1000;		//PIXELFORMAT
	if (blockwidth != 1 || blockheight != 1)
	{
		h9.dwFlags |= 0x80000;	//LINEARSIZE
		h9.dwPitchOrLinearSize =	((mips->mip[0].width+blockwidth-1)/blockwidth)*
									((mips->mip[0].height+blockheight-1)/blockheight)*
									(mips->type==PTI_3D?((mips->mip[0].depth+blockdepth-1)/blockdepth):1)*
									blockbytes;
	}
	else
	{
		h9.dwFlags |= 8;		//PITCH
		h9.dwPitchOrLinearSize = mips->mip[0].width*blockbytes;
	}
	if (mips->mipcount > 1)
		h9.dwFlags |= 0x20000;	//MIPMAPCOUNT
	h9.dwWidth = mips->mip[0].width;
	h9.dwHeight = mips->mip[0].height;
	h9.dwDepth = mips->mip[0].depth;

	h9.ddpfPixelFormat.dwSize = 32;
	h9.ddsCaps[0] = 0x1000;		//TEXTURE
	if (mips->mipcount > 1)
		h9.ddsCaps[0] |= 0x8;		//COMPLEX
	h9.ddsCaps[1] = 0;
	h10.miscflag = 0;
	h10.miscflags2 = 0;
	h9.dwMipMapCount = mips->mipcount;

	arraysize = mips->mip[0].depth;
	switch(mips->type)
	{
	case PTI_ANY:
		return false;
	case PTI_3D:
		arraysize = 1;
		h9.ddsCaps[1] |= 0x200000;	//VOLUME
		h10.resourcetype = 4;	//3d
		break;
	case PTI_CUBE_ARRAY:
		if (mips->mip[0].depth <= 1)	//in dds arraysize=1 is NOT an array, leaving us with an ambiguity issue
			return false;
		h9.dwDepth = 1;
		h10.resourcetype = 3;	//2d
		h9.ddsCaps[1] |= 0x200|0xfc00;		//CUBEMAP+faces
		h10.miscflag |= 4;//DDS_RESOURCE_MISC_TEXTURECUBE - otherwise they're basicaly just 2d_arrays
		break;
	case PTI_CUBE:
		if (mips->mip[0].depth != 6)	//wut?!?
			return false;
		h9.dwDepth = 1;
		h10.resourcetype = 3;	//2d
		h9.ddsCaps[1] |= 0x200|0xfc00;		//CUBEMAP+faces
		h10.miscflag |= 4;//DDS_RESOURCE_MISC_TEXTURECUBE - otherwise they're basicaly just 2d_arrays
		break;
	case PTI_2D_ARRAY:
		if (mips->mip[0].depth <= 1)	//in dds arraysize=1 is NOT an array, leaving us with an ambiguity issue
			return false;
		h9.dwDepth = 1;
		h10.resourcetype = 3;	//2d
		break;
	case PTI_2D:
		if (mips->mip[0].depth != 1)	//wut?!?
			return false;
		h9.dwDepth = 1;
		h10.resourcetype = 3;	//2d
		break;
	}
	if (h9.dwMipMapCount > 1)
		h9.ddsCaps[0] |= 0x400000;	//MIPMAP

	h10.arraysize = arraysize;

	h10.dxgiformat = 0;

#define DX9FOURCC(a,b,c,d)		h9.ddpfPixelFormat.dwFlags=4/*DDPF_FOURCC*/,	\
								h9.ddpfPixelFormat.dwFourCC=(a<<0)|(b<<8)|(c<<16)|(d<<24)
#define DX9FMT(bits,r,g,b,a,fl) h9.ddpfPixelFormat.dwFlags=fl,		\
								h9.ddpfPixelFormat.bitcount=bits,	\
								h9.ddpfPixelFormat.redmask=r,		\
								h9.ddpfPixelFormat.greenmask=g,		\
								h9.ddpfPixelFormat.bluemask=b,		\
								h9.ddpfPixelFormat.alphamask=a
#define DX9RGB			0x40
#define DX9RGBA			(0x40|0x1)
#define DX9LUM			0x20000
#define DX9LUMALPHA		(0x20000|0x1)
	safeswitch(mips->encoding)
	{
//	case PTI_INVALID:			h10.dxgiformat = 0x0/*DXGI_FORMAT_UNKNOWN*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x1/*DXGI_FORMAT_R32G32B32A32_TYPELESS*/;	break;
	case PTI_RGBA32F:			h10.dxgiformat = 0x2/*DXGI_FORMAT_R32G32B32A32_FLOAT*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x3/*DXGI_FORMAT_R32G32B32A32_UINT*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x4/*DXGI_FORMAT_R32G32B32A32_SINT*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x5/*DXGI_FORMAT_R32G32B32_TYPELESS*/;		break;
	case PTI_RGB32F:			h10.dxgiformat = 0x6/*DXGI_FORMAT_R32G32B32_FLOAT*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x7/*DXGI_FORMAT_R32G32B32_UINT*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x8/*DXGI_FORMAT_R32G32B32_SINT*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x9/*DXGI_FORMAT_R16G16B16A16_TYPELESS*/;	break;
	case PTI_RGBA16F:			h10.dxgiformat = 0xa/*DXGI_FORMAT_R16G16B16A16_FLOAT*/;		break;
	case PTI_RGBA16:			h10.dxgiformat = 0xb/*DXGI_FORMAT_R16G16B16A16_UNORM*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0xc/*DXGI_FORMAT_R16G16B16A16_UINT*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0xd/*DXGI_FORMAT_R16G16B16A16_SNORM*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0xe/*DXGI_FORMAT_R16G16B16A16_SINT*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0xf/*DXGI_FORMAT_R32G32_TYPELESS*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x10/*DXGI_FORMAT_R32G32_FLOAT*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x11/*DXGI_FORMAT_R32G32_UINT*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x12/*DXGI_FORMAT_R32G32_SINT*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x13/*DXGI_FORMAT_R32G8X24_TYPELESS*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x14/*DXGI_FORMAT_D32_FLOAT_S8X24_UINT*/;	break;
//	case PTI_INVALID:			h10.dxgiformat = 0x15/*DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS*/;break;
//	case PTI_INVALID:			h10.dxgiformat = 0x16/*DXGI_FORMAT_X32_TYPELESS_G8X24_UINT*/;break;
//	case PTI_INVALID:			h10.dxgiformat = 0x17/*DXGI_FORMAT_R10G10B10A2_TYPELESS*/;	break;
	case PTI_A2BGR10:			h10.dxgiformat = 0x18/*DXGI_FORMAT_R10G10B10A2_UNORM*/;		DX9FMT(32,0x000003ff,0x000ffc00,0x03ff0000,0xc0000000,DX9RGBA);	break;
//	case PTI_INVALID:			h10.dxgiformat = 0x19/*DXGI_FORMAT_R10G10B10A2_UINT*/;		break;
	case PTI_B10G11R11F:		h10.dxgiformat = 0x1a/*DXGI_FORMAT_R11G11B10_FLOAT*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x1b/*DXGI_FORMAT_R8G8B8A8_TYPELESS*/;		break;
	case PTI_RGBA8:				h10.dxgiformat = 0x1c/*DXGI_FORMAT_R8G8B8A8_UNORM*/;		DX9FMT(32,0x000000ff,0x0000ff00,0x00ff0000,0xff000000,DX9RGBA);	break;
	case PTI_RGBX8_SRGB:		//fall through...
	case PTI_RGBA8_SRGB:		h10.dxgiformat = 0x1d/*DXGI_FORMAT_R8G8B8A8_UNORM_SRGB*/;	break;
//	case PTI_INVALID:			h10.dxgiformat = 0x1e/*DXGI_FORMAT_R8G8B8A8_UINT*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x1f/*DXGI_FORMAT_R8G8B8A8_SNORM*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x20/*DXGI_FORMAT_R8G8B8A8_SINT*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x21/*DXGI_FORMAT_R16G16_TYPELESS*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x22/*DXGI_FORMAT_R16G16_FLOAT*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x23/*DXGI_FORMAT_R16G16_UNORM*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x24/*DXGI_FORMAT_R16G16_UINT*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x25/*DXGI_FORMAT_R16G16_SNORM*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x26/*DXGI_FORMAT_R16G16_SINT*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x27/*DXGI_FORMAT_R32_TYPELESS*/;			break;
	case PTI_DEPTH32:			h10.dxgiformat = 0x28/*DXGI_FORMAT_D32_FLOAT*/;				break;
	case PTI_R32F:				h10.dxgiformat = 0x29/*DXGI_FORMAT_R32_FLOAT*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x2a/*DXGI_FORMAT_R32_UINT*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x2b/*DXGI_FORMAT_R32_SINT*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x2c/*DXGI_FORMAT_R24G8_TYPELESS*/;		break;
	case PTI_DEPTH24_8:			h10.dxgiformat = 0x2d/*DXGI_FORMAT_D24_UNORM_S8_UINT*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x2e/*DXGI_FORMAT_R24_UNORM_X8_TYPELESS*/;	break;
//	case PTI_INVALID:			h10.dxgiformat = 0x2f/*DXGI_FORMAT_X24_TYPELESS_G8_UINT*/;	break;
//	case PTI_INVALID:			h10.dxgiformat = 0x30/*DXGI_FORMAT_R8G8_TYPELESS*/;			break;
	case PTI_RG8:				h10.dxgiformat = 0x31/*DXGI_FORMAT_R8G8_UNORM*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x32/*DXGI_FORMAT_R8G8_UINT*/;				break;
	case PTI_RG8_SNORM:			h10.dxgiformat = 0x33/*DXGI_FORMAT_R8G8_SNORM*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x34/*DXGI_FORMAT_R8G8_SINT*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x35/*DXGI_FORMAT_R16_TYPELESS*/;			break;
	case PTI_R16F:				h10.dxgiformat = 0x36/*DXGI_FORMAT_R16_FLOAT*/;				break;
	case PTI_DEPTH16:			h10.dxgiformat = 0x37/*DXGI_FORMAT_D16_UNORM*/;				break;
	case PTI_R16:				h10.dxgiformat = 0x38/*DXGI_FORMAT_R16_UNORM*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x39/*DXGI_FORMAT_R16_UINT*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x3a/*DXGI_FORMAT_R16_SNORM*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x3b/*DXGI_FORMAT_R16_SINT*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x3c/*DXGI_FORMAT_R8_TYPELESS*/;			break;
	case PTI_R8:				h10.dxgiformat = 0x3d/*DXGI_FORMAT_R8_UNORM*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x3e/*DXGI_FORMAT_R8_UINT*/;				break;
	case PTI_R8_SNORM:			h10.dxgiformat = 0x3f/*DXGI_FORMAT_R8_SNORM*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x40/*DXGI_FORMAT_R8_SINT*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x41/*DXGI_FORMAT_A8_UNORM*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x42/*DXGI_FORMAT_R1_UNORM*/;				break;
	case PTI_E5BGR9:			h10.dxgiformat = 0x43/*DXGI_FORMAT_R9G9B9E5_SHAREDEXP*/;	break;
//	case PTI_INVALID:			h10.dxgiformat = 0x44/*DXGI_FORMAT_R8G8_B8G8_UNORM*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x45/*DXGI_FORMAT_G8R8_G8B8_UNORM*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x46/*DXGI_FORMAT_BC1_TYPELESS*/;			break;
	case PTI_BC1_RGB:			//fall through...
	case PTI_BC1_RGBA:			h10.dxgiformat = 0x47/*DXGI_FORMAT_BC1_UNORM*/;				DX9FOURCC('D','X','T','1'); break;
	case PTI_BC1_RGB_SRGB:		//fall through...
	case PTI_BC1_RGBA_SRGB:		h10.dxgiformat = 0x48/*DXGI_FORMAT_BC1_UNORM_SRGB*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x49/*DXGI_FORMAT_BC2_TYPELESS*/;			break;
	case PTI_BC2_RGBA:			h10.dxgiformat = 0x4a/*DXGI_FORMAT_BC2_UNORM*/;				DX9FOURCC('D','X','T','3'); break;
	case PTI_BC2_RGBA_SRGB:		h10.dxgiformat = 0x4b/*DXGI_FORMAT_BC2_UNORM_SRGB*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x4c/*DXGI_FORMAT_BC3_TYPELESS*/;			break;
	case PTI_BC3_RGBA:			h10.dxgiformat = 0x4d/*DXGI_FORMAT_BC3_UNORM*/;				DX9FOURCC('D','X','T','5'); break;
	case PTI_BC3_RGBA_SRGB:		h10.dxgiformat = 0x4e/*DXGI_FORMAT_BC3_UNORM_SRGB*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x4f/*DXGI_FORMAT_BC4_TYPELESS*/;			break;
	case PTI_BC4_R:				h10.dxgiformat = 0x50/*DXGI_FORMAT_BC4_UNORM*/;				/*DX9FOURCC('B','C','4','U');*/ DX9FOURCC('A','T','I','1'); break;
	case PTI_BC4_R_SNORM:		h10.dxgiformat = 0x51/*DXGI_FORMAT_BC4_SNORM*/;				DX9FOURCC('B','C','4','S'); break;
//	case PTI_INVALID:			h10.dxgiformat = 0x52/*DXGI_FORMAT_BC5_TYPELESS*/;			break;
	case PTI_BC5_RG:			h10.dxgiformat = 0x53/*DXGI_FORMAT_BC5_UNORM*/;				/*DX9FOURCC('B','C','5','U');*/ DX9FOURCC('A','T','I','2'); break;
	case PTI_BC5_RG_SNORM:		h10.dxgiformat = 0x54/*DXGI_FORMAT_BC5_SNORM*/;				DX9FOURCC('B','C','5','S'); break;
	case PTI_RGB565:			h10.dxgiformat = 0x55/*DXGI_FORMAT_B5G6R5_UNORM*/;			DX9FMT(16,    0xf800,    0x07e0,    0x001f,    0x0000,DX9RGB);	break;
	case PTI_ARGB1555:			h10.dxgiformat = 0x56/*DXGI_FORMAT_B5G5R5A1_UNORM*/;		DX9FMT(16,    0x7c00,    0x03e0,    0x001f,    0x8000,DX9RGBA);	break;
	case PTI_BGRA8:				h10.dxgiformat = 0x57/*DXGI_FORMAT_B8G8R8A8_UNORM*/;		DX9FMT(32,0x00ff0000,0x0000ff00,0x000000ff,0xff000000,DX9RGBA);	break;
	case PTI_BGRX8:				h10.dxgiformat = 0x58/*DXGI_FORMAT_B8G8R8X8_UNORM*/;		DX9FMT(32,0x00ff0000,0x0000ff00,0x000000ff,0x00000000,DX9RGB);	break;	//WARNING: buggy in gimp (ends up alpha=0)
//	case PTI_INVALID:			h10.dxgiformat = 0x59/*DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORbreak;
//	case PTI_INVALID:			h10.dxgiformat = 0x5a/*DXGI_FORMAT_B8G8R8A8_TYPELESS*/;		break;
	case PTI_BGRA8_SRGB:		h10.dxgiformat = 0x5b/*DXGI_FORMAT_B8G8R8A8_UNORM_SRGB*/;	break;
//	case PTI_INVALID:			h10.dxgiformat = 0x5c/*DXGI_FORMAT_B8G8R8X8_TYPELESS*/;		break;
	case PTI_BGRX8_SRGB:		h10.dxgiformat = 0x5d/*DXGI_FORMAT_B8G8R8X8_UNORM_SRGB*/;	break;
//	case PTI_INVALID:			h10.dxgiformat = 0x5e/*DXGI_FORMAT_BC6H_TYPELESS*/;			break;
	case PTI_BC6_RGB_UFLOAT:	h10.dxgiformat = 0x5f/*DXGI_FORMAT_BC6H_UF16*/;				break;
	case PTI_BC6_RGB_SFLOAT:	h10.dxgiformat = 0x60/*DXGI_FORMAT_BC6H_SF16*/;				break;
//	case PTI_INVALID:			h10.dxgiformat = 0x61/*DXGI_FORMAT_BC7_TYPELESS*/;			break;
	case PTI_BC7_RGBA:			h10.dxgiformat = 0x62/*DXGI_FORMAT_BC7_UNORM*/;				break;
	case PTI_BC7_RGBA_SRGB:		h10.dxgiformat = 0x63/*DXGI_FORMAT_BC7_UNORM_SRGB*/;		break;
//	case PTI_INVALID:			h10.dxgiformat = 0x64/*DXGI_FORMAT_AYUV*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x65/*DXGI_FORMAT_Y410*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x66/*DXGI_FORMAT_Y416*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x67/*DXGI_FORMAT_NV12*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x68/*DXGI_FORMAT_P010*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x69/*DXGI_FORMAT_P016*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x6a/*DXGI_FORMAT_420_OPAQUE*/;			break;
//	case PTI_INVALID:			h10.dxgiformat = 0x6b/*DXGI_FORMAT_YUY2*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x6c/*DXGI_FORMAT_Y210*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x6d/*DXGI_FORMAT_Y216*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x6e/*DXGI_FORMAT_NV11*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x6f/*DXGI_FORMAT_AI44*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x70/*DXGI_FORMAT_IA44*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x71/*DXGI_FORMAT_P8*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x72/*DXGI_FORMAT_A8P8*/;					break;
	case PTI_ARGB4444:			h10.dxgiformat = 0x73/*DXGI_FORMAT_B4G4R4A4_UNORM*/;		DX9FMT(16,0x00000f00,0x000000f0,0x0000000f,0x0000f000,DX9RGB);	break;
//	case PTI_INVALID:			h10.dxgiformat = 0x82/*DXGI_FORMAT_P208*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x83/*DXGI_FORMAT_V208*/;					break;
//	case PTI_INVALID:			h10.dxgiformat = 0x84/*DXGI_FORMAT_V408*/;					break;
	case PTI_ASTC_4X4_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_4X4_LDR:		h10.dxgiformat = 0x86/*DXGI_FORMAT_ASTC_4X4_UNORM*/;		break;
	case PTI_ASTC_4X4_SRGB:		h10.dxgiformat = 0x87/*DXGI_FORMAT_ASTC_4X4_SRGB*/;			break;
	case PTI_ASTC_5X4_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_5X4_LDR:		h10.dxgiformat = 0x8a/*DXGI_FORMAT_ASTC_5X4_UNORM*/;		break;
	case PTI_ASTC_5X4_SRGB:		h10.dxgiformat = 0x8b/*DXGI_FORMAT_ASTC_5X4_SRGB*/;			break;
	case PTI_ASTC_5X5_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_5X5_LDR:		h10.dxgiformat = 0x8e/*DXGI_FORMAT_ASTC_5X5_UNORM*/;		break;
	case PTI_ASTC_5X5_SRGB:		h10.dxgiformat = 0x8f/*DXGI_FORMAT_ASTC_5X5_SRGB*/;			break;
	case PTI_ASTC_6X5_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_6X5_LDR:		h10.dxgiformat = 0x92/*DXGI_FORMAT_ASTC_6X5_UNORM*/;		break;
	case PTI_ASTC_6X5_SRGB:		h10.dxgiformat = 0x93/*DXGI_FORMAT_ASTC_6X5_SRGB*/;			break;
	case PTI_ASTC_6X6_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_6X6_LDR:		h10.dxgiformat = 0x96/*DXGI_FORMAT_ASTC_6X6_UNORM*/;		break;
	case PTI_ASTC_6X6_SRGB:		h10.dxgiformat = 0x97/*DXGI_FORMAT_ASTC_6X6_SRGB*/;			break;
	case PTI_ASTC_8X5_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_8X5_LDR:		h10.dxgiformat = 0x9a/*DXGI_FORMAT_ASTC_8X5_UNORM*/;		break;
	case PTI_ASTC_8X5_SRGB:		h10.dxgiformat = 0x9b/*DXGI_FORMAT_ASTC_8X5_SRGB*/;			break;
	case PTI_ASTC_8X6_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_8X6_LDR:		h10.dxgiformat = 0x9e/*DXGI_FORMAT_ASTC_8X6_UNORM*/;		break;
	case PTI_ASTC_8X6_SRGB:		h10.dxgiformat = 0x9f/*DXGI_FORMAT_ASTC_8X6_SRGB*/;			break;
	case PTI_ASTC_8X8_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_8X8_LDR:		h10.dxgiformat = 0xa2/*DXGI_FORMAT_ASTC_8X8_UNORM*/;		break;
	case PTI_ASTC_8X8_SRGB:		h10.dxgiformat = 0xa3/*DXGI_FORMAT_ASTC_8X8_SRGB*/;			break;
	case PTI_ASTC_10X5_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_10X5_LDR:		h10.dxgiformat = 0xa6/*DXGI_FORMAT_ASTC_10X5_UNORM*/;		break;
	case PTI_ASTC_10X5_SRGB:	h10.dxgiformat = 0xa7/*DXGI_FORMAT_ASTC_10X5_SRGB*/;		break;
	case PTI_ASTC_10X6_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_10X6_LDR:		h10.dxgiformat = 0xaa/*DXGI_FORMAT_ASTC_10X6_UNORM*/;		break;
	case PTI_ASTC_10X6_SRGB:	h10.dxgiformat = 0xab/*DXGI_FORMAT_ASTC_10X6_SRGB*/;		break;
	case PTI_ASTC_10X8_HDR:		//hdr allows more endpoint modes.
	case PTI_ASTC_10X8_LDR:		h10.dxgiformat = 0xae/*DXGI_FORMAT_ASTC_10X8_UNORM*/;		break;
	case PTI_ASTC_10X8_SRGB:	h10.dxgiformat = 0xaf/*DXGI_FORMAT_ASTC_10X8_SRGB*/;		break;
	case PTI_ASTC_10X10_HDR:	//hdr allows more endpoint modes.
	case PTI_ASTC_10X10_LDR:	h10.dxgiformat = 0xb2/*DXGI_FORMAT_ASTC_10X10_UNORM*/;		break;
	case PTI_ASTC_10X10_SRGB:	h10.dxgiformat = 0xb3/*DXGI_FORMAT_ASTC_10X10_SRGB*/;		break;
	case PTI_ASTC_12X10_HDR:	//hdr allows more endpoint modes.
	case PTI_ASTC_12X10_LDR:	h10.dxgiformat = 0xb6/*DXGI_FORMAT_ASTC_12X10_UNORM*/;		break;
	case PTI_ASTC_12X10_SRGB:	h10.dxgiformat = 0xb7/*DXGI_FORMAT_ASTC_12X10_SRGB*/;		break;
	case PTI_ASTC_12X12_HDR:	//hdr allows more endpoint modes.
	case PTI_ASTC_12X12_LDR:	h10.dxgiformat = 0xba/*DXGI_FORMAT_ASTC_12X12_UNORM*/;		break;
	case PTI_ASTC_12X12_SRGB:	h10.dxgiformat = 0xbb/*DXGI_FORMAT_ASTC_12X12_SRGB*/;		break;
#ifdef ASTC3D
	case PTI_ASTC_3X3X3_HDR:
	case PTI_ASTC_4X3X3_HDR:
	case PTI_ASTC_4X4X3_HDR:
	case PTI_ASTC_4X4X4_HDR:
	case PTI_ASTC_5X4X4_HDR:
	case PTI_ASTC_5X5X4_HDR:
	case PTI_ASTC_5X5X5_HDR:
	case PTI_ASTC_6X5X5_HDR:
	case PTI_ASTC_6X6X5_HDR:
	case PTI_ASTC_6X6X6_HDR:
	case PTI_ASTC_3X3X3_LDR:
	case PTI_ASTC_4X3X3_LDR:
	case PTI_ASTC_4X4X3_LDR:
	case PTI_ASTC_4X4X4_LDR:
	case PTI_ASTC_5X4X4_LDR:
	case PTI_ASTC_5X5X4_LDR:
	case PTI_ASTC_5X5X5_LDR:
	case PTI_ASTC_6X5X5_LDR:
	case PTI_ASTC_6X6X5_LDR:
	case PTI_ASTC_6X6X6_LDR:
	case PTI_ASTC_3X3X3_SRGB:
	case PTI_ASTC_4X3X3_SRGB:
	case PTI_ASTC_4X4X3_SRGB:
	case PTI_ASTC_4X4X4_SRGB:
	case PTI_ASTC_5X4X4_SRGB:
	case PTI_ASTC_5X5X4_SRGB:
	case PTI_ASTC_5X5X5_SRGB:
	case PTI_ASTC_6X5X5_SRGB:
	case PTI_ASTC_6X6X5_SRGB:
	case PTI_ASTC_6X6X6_SRGB:	return false;	//no dxgi format assigned that we know of
#endif

	case PTI_RGBX8:				DX9FMT(32,0x000000ff,0x0000ff00,0x00ff0000,0x00000000,DX9RGB);	break;	//WARNING: buggy in gimp (ends up alpha=0)
	case PTI_RGB8:				DX9FMT(24,0x000000ff,0x0000ff00,0x00ff0000,0x00000000,DX9RGB);	break;
	case PTI_BGR8:				DX9FMT(24,0x00ff0000,0x0000ff00,0x000000ff,0x00000000,DX9RGB);	break;
	case PTI_L8:				DX9FMT(8,0x000000ff,0x00000000,0x00000000,0x00000000,DX9LUM);	break;
	case PTI_L8A8:				DX9FMT(16,0x000000ff,0x00000000,0x00000000,0x0000ff00,DX9LUMALPHA);	break;
	case PTI_RGBA5551:			DX9FMT(16,0x0000f800,0x000007c0,0x0000003e,0x00000001,DX9RGBA);	break;	//WARNING: buggy in gimp (ends up greyscale)
	case PTI_RGBA4444:			DX9FMT(16,0x0000f000,0x00000f00,0x000000f0,0x0000000f,DX9RGBA);	break;	//WARNING: buggy in gimp (ends up greyscale)

	case PTI_ETC1_RGB8:			//fall through (etc2 is backwards compatible)
	case PTI_ETC2_RGB8:			DX9FOURCC('E','T','C','2');	break;	//not an official format, but we can understand it
	case PTI_ETC2_RGB8_SRGB:
	case PTI_ETC2_RGB8A1:
	case PTI_ETC2_RGB8A1_SRGB:
	case PTI_ETC2_RGB8A8:
	case PTI_ETC2_RGB8A8_SRGB:
	case PTI_EAC_R11:
	case PTI_EAC_R11_SNORM:
	case PTI_EAC_RG11:
	case PTI_EAC_RG11_SNORM:	return false;	//unsupported

	case PTI_L8_SRGB:			return false;	//unsupported
	case PTI_L8A8_SRGB:			return false;	//unsupported
	case PTI_RGB8_SRGB:			return false;	//unsupported
	case PTI_BGR8_SRGB:			return false;	//unsupported
	case PTI_DEPTH24:			return false;	//unsupported, should fall back on dx9 formats.
	case PTI_P8:				return false;	//unsupported, technically R8_UNORM but would load back in wrongly.

#ifdef PVRQUAKE
	case PTI_ARGB1555_VQ:
	case PTI_ARGB1555_TWIDDLED:
	case PTI_ARGB1555_VQ_TWIDDLED:
	case PTI_RGB565_VQ:
	case PTI_RGB565_TWIDDLED:
	case PTI_RGB565_VQ_TWIDDLED:
	case PTI_ARGB4444_VQ:
	case PTI_ARGB4444_TWIDDLED:
	case PTI_ARGB4444_VQ_TWIDDLED:
	case PTI_P4_TWIDDLED:
	case PTI_P8_TWIDDLED:
#endif

#ifdef FTE_TARGET_WEB
	case PTI_WHOLEFILE:
#endif
	case PTI_EMULATED:
	case PTI_MAX:
		return false;

	safedefault:	//don't enable in debug builds, so we get warnings for any cases being missed.
		return false;
	}

	//truncate the mip chain if they're dodgy sizes.
	for (mipnum = 1; mipnum < h9.dwMipMapCount; mipnum++)
	{
		size_t m = mipnum;
		size_t p = (mipnum-1);
		if (mips->mip[m].width != max(1,(mips->mip[p].width)>>1) ||
			mips->mip[m].height != max(1,(mips->mip[p].height)>>1))
		{
			h9.dwMipMapCount = mipnum;
			break;
		}
	}

	if (strchr(filename, '*') || strchr(filename, ':'))
		return false;

	if (h9.ddpfPixelFormat.dwFlags && h10.arraysize == 1)
		h10.dxgiformat = 0;	//skip the dx10 header if we can express it as bitmasks. this generally gives better support in external tools (especially gimp)
	else if (h10.dxgiformat)
	{
		h9.ddpfPixelFormat.dwFlags = 0;	//don't get confused. always one or the other.
		DX9FOURCC('D','X','1','0');
	}
	else
		return false;	//legacy-only arrays are not supported.

	file = FS_OpenVFS(filename, "wb", FS_GAMEONLY);
	if (!file)
		return false;
	VFS_WRITE(file, "DDS ", 4);
	for (endian = (int*)&h9; endian < (int*)(&h9+1); endian++)
		*endian = LittleLong(*endian);
	VFS_WRITE(file, &h9, sizeof(h9));
	if (h10.dxgiformat)
	{
		for (endian = (int*)&h10; endian < (int*)(&h10+1); endian++)
			*endian = LittleLong(*endian);
		VFS_WRITE(file, &h10, sizeof(h10));
	}

	//our internal state uses width*height*layers for each mip level (gl-friendly).
	//DDS requires a0m0, a0m1, a1m0, a1m1, so reorder with two nested loops
	for (a = 0; a < arraysize; a++)
	{
		for (mipnum = 0; mipnum < h9.dwMipMapCount; mipnum++)
		{
			size_t sz = mips->mip[mipnum].datasize / arraysize;
			VFS_WRITE(file, (qbyte*)mips->mip[mipnum].data + sz*a, sz);
		}
	}

	VFS_CLOSE(file);
	return true;
}

#endif // IMAGEFMT_DDS
