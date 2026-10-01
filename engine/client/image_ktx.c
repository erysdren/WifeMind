#include "image.h"

#ifdef IMAGEFMT_KTX

typedef struct
{
	char magic[12];
	unsigned int endianness;

	unsigned int gltype;
	unsigned int gltypesize;
	unsigned int glformat;
	unsigned int glinternalformat;

	unsigned int glbaseinternalformat;
	unsigned int pixelwidth;
	unsigned int pixelheight;
	unsigned int pixeldepth;

	unsigned int numberofarrayelements;
	unsigned int numberoffaces;
	unsigned int numberofmipmaplevels;
	unsigned int bytesofkeyvaluedata;
} ktxheader_t;
qboolean Image_WriteKTXFile(const char *filename, enum fs_relative fsroot, struct pendingtextureinfo *mips)
{
	unsigned int bb,bw,bh,bd;
	vfsfile_t *file;
	ktxheader_t header = {{0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A}, 0x04030201/*endianness*/,
		0/*type*/, 1/*typesize*/, 0/*format*/, 0/*internalformat*/,
		0/*base*/, mips->mip[0].width, mips->mip[0].height, 0/*depth*/,
		0/*array elements*/, 1, mips->mipcount, 0/*kvdatasize*/};
	size_t mipnum;
	if (mips->type==PTI_CUBE_ARRAY)
	{
		if (!mips->mip[0].depth || mips->mip[0].depth % 6)
		{
			Con_Printf("Image_WriteKTXFile: malformed cube\n");
			return false;	//malformed...
		}
		header.numberoffaces = 6;
		header.numberofarrayelements = mips->mip[0].depth/6;
	}
	else if (mips->type==PTI_CUBE)
	{
		if (mips->mip[0].depth != 6)
		{
			Con_Printf("Image_WriteKTXFile: malformed cube\n");
			return false;	//malformed...
		}
		header.numberofarrayelements = 0;
		header.numberoffaces = 6;
	}
	else if (mips->type==PTI_2D_ARRAY)
	{
		if (!mips->mip[0].depth)
			return false;
		header.numberofarrayelements = mips->mip[0].depth;
	}
	else if (mips->type == PTI_3D)
		header.pixeldepth = mips->mip[0].depth;
	else if (mips->type == PTI_2D)
	{
		if (mips->mip[0].depth != 1)
			return false;
	}
	else
	{
		Con_Printf("Image_WriteKTXFile: unsupported texture type\n");
		return false;
	}

	Image_BlockSizeForEncoding(mips->encoding, &bb, &bw, &bh, &bd);

	safeswitch(mips->encoding)
	{
	case PTI_ETC1_RGB8:			header.glinternalformat = 0x8D64/*GL_ETC1_RGB8_OES*/; break;
	case PTI_ETC2_RGB8:			header.glinternalformat = 0x9274/*GL_COMPRESSED_RGB8_ETC2*/; break;
	case PTI_ETC2_RGB8_SRGB:	header.glinternalformat = 0x9275/*GL_COMPRESSED_SRGB8_ETC2*/; break;
	case PTI_ETC2_RGB8A1:		header.glinternalformat = 0x9276/*GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2*/; break;
	case PTI_ETC2_RGB8A1_SRGB:	header.glinternalformat = 0x9277/*GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2*/; break;
	case PTI_ETC2_RGB8A8:		header.glinternalformat = 0x9278/*GL_COMPRESSED_RGBA8_ETC2_EAC*/; break;
	case PTI_ETC2_RGB8A8_SRGB:	header.glinternalformat = 0x9279/*GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC*/; break;
	case PTI_EAC_R11:			header.glinternalformat = 0x9270/*GL_COMPRESSED_R11_EAC*/; break;
	case PTI_EAC_R11_SNORM:		header.glinternalformat = 0x9271/*GL_COMPRESSED_SIGNED_R11_EAC*/; break;
	case PTI_EAC_RG11:			header.glinternalformat = 0x9272/*GL_COMPRESSED_RG11_EAC*/; break;
	case PTI_EAC_RG11_SNORM:	header.glinternalformat = 0x9273/*GL_COMPRESSED_SIGNED_RG11_EAC*/; break;
	case PTI_BC1_RGB:			header.glinternalformat = 0x83F0/*GL_COMPRESSED_RGB_S3TC_DXT1_EXT*/; break;
	case PTI_BC1_RGB_SRGB:		header.glinternalformat = 0x8C4C/*GL_COMPRESSED_SRGB_S3TC_DXT1_EXT*/; break;
	case PTI_BC1_RGBA:			header.glinternalformat = 0x83F1/*GL_COMPRESSED_RGBA_S3TC_DXT1_EXT*/; break;
	case PTI_BC1_RGBA_SRGB:		header.glinternalformat = 0x8C4D/*GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT*/; break;
	case PTI_BC2_RGBA:			header.glinternalformat = 0x83F2/*GL_COMPRESSED_RGBA_S3TC_DXT3_EXT*/; break;
	case PTI_BC2_RGBA_SRGB:		header.glinternalformat = 0x8C4E/*GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT*/; break;
	case PTI_BC3_RGBA:			header.glinternalformat = 0x83F3/*GL_COMPRESSED_RGBA_S3TC_DXT5_EXT*/; break;
	case PTI_BC3_RGBA_SRGB:		header.glinternalformat = 0x8C4F/*GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT*/; break;
	case PTI_BC4_R_SNORM:		header.glinternalformat = 0x8DBC/*GL_COMPRESSED_SIGNED_RED_RGTC1*/; break;
	case PTI_BC4_R:				header.glinternalformat = 0x8DBB/*GL_COMPRESSED_RED_RGTC1*/; break;
	case PTI_BC5_RG_SNORM:		header.glinternalformat = 0x8DBE/*GL_COMPRESSED_SIGNED_RG_RGTC2*/; break;
	case PTI_BC5_RG:			header.glinternalformat = 0x8DBD/*GL_COMPRESSED_RG_RGTC2*/; break;
	case PTI_BC6_RGB_UFLOAT:	header.glinternalformat = 0x8E8F/*GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB*/; break;
	case PTI_BC6_RGB_SFLOAT:	header.glinternalformat = 0x8E8E/*GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB*/; break;
	case PTI_BC7_RGBA:			header.glinternalformat = 0x8E8C/*GL_COMPRESSED_RGBA_BPTC_UNORM_ARB*/; break;
	case PTI_BC7_RGBA_SRGB:		header.glinternalformat = 0x8E8D/*GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB*/; break;
	case PTI_ASTC_4X4_HDR:		//sadly gl/ktx does not distinguish between ldr+hdr, which presents problems that will have to be handled by the loader.
	case PTI_ASTC_4X4_LDR:		header.glinternalformat = 0x93B0/*GL_COMPRESSED_RGBA_ASTC_4x4_KHR*/; break;
	case PTI_ASTC_5X4_HDR:
	case PTI_ASTC_5X4_LDR:		header.glinternalformat = 0x93B1/*GL_COMPRESSED_RGBA_ASTC_5x4_KHR*/; break;
	case PTI_ASTC_5X5_HDR:
	case PTI_ASTC_5X5_LDR:		header.glinternalformat = 0x93B2/*GL_COMPRESSED_RGBA_ASTC_5x5_KHR*/; break;
	case PTI_ASTC_6X5_HDR:
	case PTI_ASTC_6X5_LDR:		header.glinternalformat = 0x93B3/*GL_COMPRESSED_RGBA_ASTC_6x5_KHR*/; break;
	case PTI_ASTC_6X6_HDR:
	case PTI_ASTC_6X6_LDR:		header.glinternalformat = 0x93B4/*GL_COMPRESSED_RGBA_ASTC_6x6_KHR*/; break;
	case PTI_ASTC_8X5_HDR:
	case PTI_ASTC_8X5_LDR:		header.glinternalformat = 0x93B5/*GL_COMPRESSED_RGBA_ASTC_8x5_KHR*/; break;
	case PTI_ASTC_8X6_HDR:
	case PTI_ASTC_8X6_LDR:		header.glinternalformat = 0x93B6/*GL_COMPRESSED_RGBA_ASTC_8x6_KHR*/; break;
	case PTI_ASTC_8X8_HDR:
	case PTI_ASTC_8X8_LDR:		header.glinternalformat = 0x93B7/*GL_COMPRESSED_RGBA_ASTC_8x8_KHR*/; break;
	case PTI_ASTC_10X5_HDR:
	case PTI_ASTC_10X5_LDR:		header.glinternalformat = 0x93B8/*GL_COMPRESSED_RGBA_ASTC_10x5_KHR*/; break;
	case PTI_ASTC_10X6_HDR:
	case PTI_ASTC_10X6_LDR:		header.glinternalformat = 0x93B9/*GL_COMPRESSED_RGBA_ASTC_10x6_KHR*/; break;
	case PTI_ASTC_10X8_HDR:
	case PTI_ASTC_10X8_LDR:		header.glinternalformat = 0x93BA/*GL_COMPRESSED_RGBA_ASTC_10x8_KHR*/; break;
	case PTI_ASTC_10X10_HDR:
	case PTI_ASTC_10X10_LDR:	header.glinternalformat = 0x93BB/*GL_COMPRESSED_RGBA_ASTC_10x10_KHR*/; break;
	case PTI_ASTC_12X10_HDR:
	case PTI_ASTC_12X10_LDR:	header.glinternalformat = 0x93BC/*GL_COMPRESSED_RGBA_ASTC_12x10_KHR*/; break;
	case PTI_ASTC_12X12_HDR:
	case PTI_ASTC_12X12_LDR:	header.glinternalformat = 0x93BD/*GL_COMPRESSED_RGBA_ASTC_12x12_KHR*/; break;
	case PTI_ASTC_4X4_SRGB:		header.glinternalformat = 0x93D0/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR*/; break;
	case PTI_ASTC_5X4_SRGB:		header.glinternalformat = 0x93D1/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR*/; break;
	case PTI_ASTC_5X5_SRGB:		header.glinternalformat = 0x93D2/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR*/; break;
	case PTI_ASTC_6X5_SRGB:		header.glinternalformat = 0x93D3/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR*/; break;
	case PTI_ASTC_6X6_SRGB:		header.glinternalformat = 0x93D4/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR*/; break;
	case PTI_ASTC_8X5_SRGB:		header.glinternalformat = 0x93D5/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR*/; break;
	case PTI_ASTC_8X6_SRGB:		header.glinternalformat = 0x93D6/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR*/; break;
	case PTI_ASTC_8X8_SRGB:		header.glinternalformat = 0x93D7/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR*/; break;
	case PTI_ASTC_10X5_SRGB:	header.glinternalformat = 0x93D8/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR*/; break;
	case PTI_ASTC_10X6_SRGB:	header.glinternalformat = 0x93D9/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR*/; break;
	case PTI_ASTC_10X8_SRGB:	header.glinternalformat = 0x93DA/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR*/; break;
	case PTI_ASTC_10X10_SRGB:	header.glinternalformat = 0x93DB/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR*/; break;
	case PTI_ASTC_12X10_SRGB:	header.glinternalformat = 0x93DC/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR*/; break;
	case PTI_ASTC_12X12_SRGB:	header.glinternalformat = 0x93DD/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR*/; break;
#ifdef ASTC3D
	case PTI_ASTC_3X3X3_HDR:
	case PTI_ASTC_3X3X3_LDR:	header.glinternalformat = 0x93C0/*GL_COMPRESSED_RGBA_ASTC_3x3x3_OES*/; break;
	case PTI_ASTC_4X3X3_HDR:
	case PTI_ASTC_4X3X3_LDR:	header.glinternalformat = 0x93C1/*GL_COMPRESSED_RGBA_ASTC_4x3x3_OES*/; break;
	case PTI_ASTC_4X4X3_HDR:
	case PTI_ASTC_4X4X3_LDR:	header.glinternalformat = 0x93C2/*GL_COMPRESSED_RGBA_ASTC_4x4x3_OES*/; break;
	case PTI_ASTC_4X4X4_HDR:
	case PTI_ASTC_4X4X4_LDR:	header.glinternalformat = 0x93C3/*GL_COMPRESSED_RGBA_ASTC_4x4x5_OES*/; break;
	case PTI_ASTC_5X4X4_HDR:
	case PTI_ASTC_5X4X4_LDR:	header.glinternalformat = 0x93C4/*GL_COMPRESSED_RGBA_ASTC_5x4x4_OES*/; break;
	case PTI_ASTC_5X5X4_HDR:
	case PTI_ASTC_5X5X4_LDR:	header.glinternalformat = 0x93C5/*GL_COMPRESSED_RGBA_ASTC_5x5x4_OES*/; break;
	case PTI_ASTC_5X5X5_HDR:
	case PTI_ASTC_5X5X5_LDR:	header.glinternalformat = 0x93C6/*GL_COMPRESSED_RGBA_ASTC_5x5x5_OES*/; break;
	case PTI_ASTC_6X5X5_HDR:
	case PTI_ASTC_6X5X5_LDR:	header.glinternalformat = 0x93C7/*GL_COMPRESSED_RGBA_ASTC_6x5x5_OES*/; break;
	case PTI_ASTC_6X6X5_HDR:
	case PTI_ASTC_6X6X5_LDR:	header.glinternalformat = 0x93C8/*GL_COMPRESSED_RGBA_ASTC_6x6x5_OES*/; break;
	case PTI_ASTC_6X6X6_HDR:
	case PTI_ASTC_6X6X6_LDR:	header.glinternalformat = 0x93C9/*GL_COMPRESSED_RGBA_ASTC_6x6x6_OES*/; break;
	case PTI_ASTC_3X3X3_SRGB:	header.glinternalformat = 0x93E0/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_3x3x3_OES*/; break;
	case PTI_ASTC_4X3X3_SRGB:	header.glinternalformat = 0x93E1/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x3x3_OES*/; break;
	case PTI_ASTC_4X4X3_SRGB:	header.glinternalformat = 0x93E2/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x3_OES*/; break;
	case PTI_ASTC_4X4X4_SRGB:	header.glinternalformat = 0x93E3/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4x4_OES*/; break;
	case PTI_ASTC_5X4X4_SRGB:	header.glinternalformat = 0x93E4/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4x4_OES*/; break;
	case PTI_ASTC_5X5X4_SRGB:	header.glinternalformat = 0x93E5/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x4_OES*/; break;
	case PTI_ASTC_5X5X5_SRGB:	header.glinternalformat = 0x93E6/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5x5_OES*/; break;
	case PTI_ASTC_6X5X5_SRGB:	header.glinternalformat = 0x93E7/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5x5_OES*/; break;
	case PTI_ASTC_6X6X5_SRGB:	header.glinternalformat = 0x93E8/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x5_OES*/; break;
	case PTI_ASTC_6X6X6_SRGB:	header.glinternalformat = 0x93E9/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6x6_OES*/; break;
#endif

	case PTI_BGRA8:				header.glinternalformat = 0x8058/*GL_RGBA8*/;				header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x80E1/*GL_BGRA*/;			header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_RGBA8:				header.glinternalformat = 0x8058/*GL_RGBA8*/;				header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x1908/*GL_RGBA*/;			header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_BGRA8_SRGB:		header.glinternalformat = 0x8C43/*GL_SRGB8_ALPHA8*/;		header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x80E1/*GL_BGRA*/;			header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_RGBA8_SRGB:		header.glinternalformat = 0x8C43/*GL_SRGB8_ALPHA8*/;		header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x1908/*GL_RGBA*/;			header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_L8:				header.glinternalformat = 0x8040/*GL_LUMINANCE8*/;			header.glbaseinternalformat = 0x1909/*GL_LUMINANCE*/;		header.glformat = 0x1909/*GL_LUMINANCE*/;		header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_L8A8:				header.glinternalformat = 0x8045/*GL_LUMINANCE8_ALPHA8*/;	header.glbaseinternalformat = 0x190A/*GL_LUMINANCE_ALPHA*/;	header.glformat = 0x190A/*GL_LUMINANCE_ALPHA*/;	header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_L8_SRGB:			header.glinternalformat = 0x8C47/*GL_SLUMINANCE8*/;			header.glbaseinternalformat = 0x1909/*GL_LUMINANCE*/;		header.glformat = 0x1909/*GL_LUMINANCE*/;		header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_L8A8_SRGB:			header.glinternalformat = 0x8C45/*GL_SLUMINANCE8_ALPHA8*/;	header.glbaseinternalformat = 0x190A/*GL_LUMINANCE_ALPHA*/;	header.glformat = 0x190A/*GL_LUMINANCE_ALPHA*/;	header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_RGB8:				header.glinternalformat = 0x8051/*GL_RGB8*/;				header.glbaseinternalformat = 0x1907/*GL_RGB*/;				header.glformat = 0x1907/*GL_RGB*/;				header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_BGR8:				header.glinternalformat = 0x8051/*GL_RGB8*/;				header.glbaseinternalformat = 0x1907/*GL_RGB*/;				header.glformat = 0x80E0/*GL_BGR*/;				header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_RGB8_SRGB:			header.glinternalformat = 0x8C41/*GL_SRGB8*/;				header.glbaseinternalformat = 0x1907/*GL_RGB*/;				header.glformat = 0x1907/*GL_RGB*/;				header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_BGR8_SRGB:			header.glinternalformat = 0x8C41/*GL_SRGB8*/;				header.glbaseinternalformat = 0x1907/*GL_RGB*/;				header.glformat = 0x80E0/*GL_BGR*/;				header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_R16:				header.glinternalformat = 0x822A/*GL_R16*/;					header.glbaseinternalformat = 0x1903/*GL_RED*/;				header.glformat = 0x1903/*GL_RED*/;				header.gltype = 0x1403/*GL_UNSIGNED_SHORT*/;				header.gltypesize = 2; break;
	case PTI_RGBA16:			header.glinternalformat = 0x805B/*GL_RGBA16*/;				header.glbaseinternalformat = 0x1903/*GL_RED*/;				header.glformat = 0x1903/*GL_RED*/;				header.gltype = 0x1403/*GL_UNSIGNED_SHORT*/;				header.gltypesize = 2; break;
	case PTI_R16F:				header.glinternalformat = 0x822D/*GL_R16F*/;				header.glbaseinternalformat = 0x1903/*GL_RED*/;				header.glformat = 0x1903/*GL_RED*/;				header.gltype = 0x140B/*GL_HALF_FLOAT*/;					header.gltypesize = 2; break;
	case PTI_R32F:				header.glinternalformat = 0x822E/*GL_R32F*/;				header.glbaseinternalformat = 0x1903/*GL_RED*/;				header.glformat = 0x1903/*GL_RED*/;				header.gltype = 0x1406/*GL_FLOAT*/;							header.gltypesize = 4; break;
	case PTI_RGBA16F:			header.glinternalformat = 0x881A/*GL_RGBA16F*/;				header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x1908/*GL_RGBA*/;			header.gltype = 0x140B/*GL_HALF_FLOAT*/;					header.gltypesize = 2; break;
	case PTI_RGB32F:			header.glinternalformat = 0x8815/*GL_RGB32F*/;				header.glbaseinternalformat = 0x1907/*GL_RGB*/;				header.glformat = 0x1907/*GL_RGB*/;				header.gltype = 0x1406/*GL_FLOAT*/;							header.gltypesize = 4; break;
	case PTI_RGBA32F:			header.glinternalformat = 0x8814/*GL_RGBA32F*/;				header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x1908/*GL_RGBA*/;			header.gltype = 0x1406/*GL_FLOAT*/;							header.gltypesize = 4; break;
	case PTI_A2BGR10:			header.glinternalformat = 0x8059/*GL_RGB10_A2*/;			header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x1908/*GL_RGBA*/;			header.gltype = 0x8368/*GL_UNSIGNED_INT_2_10_10_10_REV*/;	header.gltypesize = 4; break;
	case PTI_E5BGR9:			header.glinternalformat = 0x8C3D/*GL_RGB9_E5*/;				header.glbaseinternalformat = 0x8C3D/*GL_RGB9_E5*/;			header.glformat = 0x1907/*GL_RGB*/;				header.gltype = 0x8C3E/*GL_UNSIGNED_INT_5_9_9_9_REV*/;		header.gltypesize = 4; break;
	case PTI_B10G11R11F:		header.glinternalformat = 0x8C3A/*GL_R11F_G11F_B10F*/;		header.glbaseinternalformat = 0x8C3D/*GL_R11F_G11F_B10F*/;	header.glformat = 0x1907/*GL_RGB*/;				header.gltype = 0x8C3B/*GL_UNSIGNED_INT_10_11_11_REV*/;		header.gltypesize = 4; break;
	case PTI_P8:
	case PTI_R8:				header.glinternalformat = 0x8229/*GL_R8*/;					header.glbaseinternalformat = 0x1903/*GL_RED*/;				header.glformat = 0x1903/*GL_RED*/;				header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_RG8:				header.glinternalformat = 0x822B/*GL_RG8*/;					header.glbaseinternalformat = 0x8227/*GL_RG*/;				header.glformat = 0x8227/*GL_RG*/;				header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_R8_SNORM:			header.glinternalformat = 0x8F94/*GL_R8_SNORM*/;			header.glbaseinternalformat = 0x1903/*GL_RED*/;				header.glformat = 0x1903/*GL_RED*/;				header.gltype = 0x1400/*GL_BYTE*/;							header.gltypesize = 1; break;
	case PTI_RG8_SNORM:			header.glinternalformat = 0x8F95/*GL_RG8_SNORM*/;			header.glbaseinternalformat = 0x8227/*GL_RG*/;				header.glformat = 0x8227/*GL_RG*/;				header.gltype = 0x1400/*GL_BYTE*/;							header.gltypesize = 1; break;
	case PTI_BGRX8:				header.glinternalformat = 0x8051/*GL_RGB8*/;				header.glbaseinternalformat = 0x1907/*GL_RGB*/;				header.glformat = 0x80E1/*GL_BGRA*/;			header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_RGBX8:				header.glinternalformat = 0x8051/*GL_RGB8*/;				header.glbaseinternalformat = 0x1907/*GL_RGB*/;				header.glformat = 0x1908/*GL_RGBA*/;			header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_BGRX8_SRGB:		header.glinternalformat = 0x8C41/*GL_SRGB8*/;				header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x80E1/*GL_BGRA*/;			header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_RGBX8_SRGB:		header.glinternalformat = 0x8C41/*GL_SRGB8*/;				header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x1908/*GL_RGBA*/;			header.gltype = 0x1401/*GL_UNSIGNED_BYTE*/;					header.gltypesize = 1; break;
	case PTI_RGB565:			header.glinternalformat = 0x8D62/*GL_RGB565*/;				header.glbaseinternalformat = 0x1907/*GL_RGB*/;				header.glformat = 0x1907/*GL_RGB*/;				header.gltype = 0x8363/*GL_UNSIGNED_SHORT_5_6_5*/;			header.gltypesize = 2; break;
	case PTI_RGBA4444:			header.glinternalformat = 0x8056/*GL_RGBA4*/;				header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x1908/*GL_RGBA*/;			header.gltype = 0x8033/*GL_UNSIGNED_SHORT_4_4_4_4*/;		header.gltypesize = 2; break;
	case PTI_ARGB4444:			header.glinternalformat = 0x8056/*GL_RGBA4*/;				header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x80E1/*GL_BGRA*/;			header.gltype = 0x8365/*GL_UNSIGNED_SHORT_4_4_4_4_REV*/;	header.gltypesize = 2; break;
	case PTI_RGBA5551:			header.glinternalformat = 0x8057/*GL_RGB5_A1*/;				header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x1908/*GL_RGBA*/;			header.gltype = 0x8034/*GL_UNSIGNED_SHORT_5_5_5_1*/;		header.gltypesize = 2; break;
	case PTI_ARGB1555:			header.glinternalformat = 0x8057/*GL_RGB5_A1*/;				header.glbaseinternalformat = 0x1908/*GL_RGBA*/;			header.glformat = 0x80E1/*GL_BGRA*/;			header.gltype = 0x8366/*GL_UNSIGNED_SHORT_1_5_5_5_REV*/;	header.gltypesize = 2; break;
	case PTI_DEPTH16:			header.glinternalformat = 0x81A5/*GL_DEPTH_COMPONENT16*/;	header.glbaseinternalformat = 0x1902/*GL_DEPTH_COMPONENT*/;	header.glformat = 0x1902/*GL_DEPTH_COMPONENT*/;	header.gltype = 0x1403/*GL_UNSIGNED_SHORT*/;				header.gltypesize = 2; break;
	case PTI_DEPTH24:			header.glinternalformat = 0x81A6/*GL_DEPTH_COMPONENT24*/;	header.glbaseinternalformat = 0x1902/*GL_DEPTH_COMPONENT*/;	header.glformat = 0x1902/*GL_DEPTH_COMPONENT*/;	header.gltype = 0x1405/*GL_UNSIGNED_INT*/;					header.gltypesize = 3; break;
	case PTI_DEPTH32:			header.glinternalformat = 0x81A7/*GL_DEPTH_COMPONENT32*/;	header.glbaseinternalformat = 0x1902/*GL_DEPTH_COMPONENT*/;	header.glformat = 0x1902/*GL_DEPTH_COMPONENT*/;	header.gltype = 0x1406/*GL_FLOAT*/;							header.gltypesize = 4; break;
	case PTI_DEPTH24_8:			header.glinternalformat = 0x88F0/*GL_DEPTH24_STENCIL8*/;	header.glbaseinternalformat = 0x84F9/*GL_DEPTH_STENCIL*/;	header.glformat = 0x84F9/*GL_DEPTH_STENCIL*/;	header.gltype = 0x84FA/*GL_UNSIGNED_INT_24_8*/;				header.gltypesize = 4; break;

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

	safedefault:
		return false;
	}

	if (strchr(filename, '*') || strchr(filename, ':'))
		return false;

	file = FS_OpenVFS(filename, "wb", fsroot);
	if (!file)
		return false;
	VFS_WRITE(file, &header, sizeof(header));

	for (mipnum = 0; mipnum < mips->mipcount; )
	{
		unsigned int sz;
		//translate to blocks
		unsigned int browbytes = bb * ((mips->mip[mipnum].width+bw-1)/bh);
		unsigned int padbytes = (browbytes&3)?4-(browbytes&3):0;
		unsigned int brows = (mips->mip[mipnum].height+bh-1)/bh;
		unsigned int blayers = (mips->mip[mipnum].depth+bd-1)/bd;
		if (mips->mip[mipnum].datasize != browbytes*brows*blayers)
		{	//should probably be a sys_error
			Con_Printf("WriteKTX mip %u missized\n", (unsigned)mipnum);
			VFS_CLOSE(file);
			return false;
		}
		switch(mips->type)
		{
		case PTI_ANY:
			VFS_CLOSE(file);
			return false;
		case PTI_CUBE:	//special case, size is per-face
			sz = (browbytes+padbytes) * brows;
			break;
		case PTI_2D:
		case PTI_2D_ARRAY:
		case PTI_CUBE_ARRAY:
		case PTI_3D:
			sz = (browbytes+padbytes) * brows * blayers;
			break;
		}
		VFS_WRITE(file, &sz, 4);
		brows *= blayers;
		if (padbytes)
		{
			unsigned int pad = 0, y;
			for (y = 0; y < brows; y++)
			{
				VFS_WRITE(file, (qbyte*)mips->mip[mipnum].data + browbytes*y, browbytes);
				VFS_WRITE(file, &pad, 4-(browbytes&3));
			}
		}
		else
			VFS_WRITE(file, mips->mip[mipnum].data, browbytes*brows);
		mipnum++;
	}

	VFS_CLOSE(file);
	return true;
}

#define LongSwap(i) (((i&0xff000000) >> 24)|((i&0x00ff0000) >> 8)|((i&0x0000ff00) << 8)|((i&0x000000ff) << 24))
#define ShortSwap(i) (((i&0xff00) >> 8)|((i&0x00ff) << 8))
static struct pendingtextureinfo *Image_ReadKTX1File(unsigned int flags, const char *fname, qbyte *filedata, size_t filesize)
{
	static const char magic[12] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x31, 0x31, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
	ktxheader_t header;
	int nummips;
	int mipnum;
	int datasize;
	unsigned int *swap, w, h, d, f, l, browbytes,padbytes,y,x,rows;
	struct pendingtextureinfo *mips;
	int encoding = TF_INVALID;
	const qbyte *fileend = filedata + filesize;

	unsigned int blockwidth, blockheight, blockdepth, blockbytes;

	if (filesize < sizeof(ktxheader_t) || memcmp(filedata, magic, sizeof(magic)))
		return NULL;	//not a ktx file

	header = *(const ktxheader_t*)filedata;
	if (header.endianness == 0x01020304)
	{	//swap the rest of the header.
		for (swap = &header.endianness; swap < (unsigned int*)(&header+1); swap++)
			*swap = LongSwap(*swap);
	}
	else if (header.endianness != 0x04030201)
		return NULL;

	nummips = header.numberofmipmaplevels;
	if (nummips < 1)
		nummips = 1;

//	if (header->numberofarrayelements != 0)
//		return NULL;	//don't support array textures
	if (header.numberoffaces == 1)
		;	//non-cubemap
	else if (header.numberoffaces == 6)
	{
		if (header.numberofarrayelements != 0)
			return NULL;	//don't support array textures

		if (header.pixeldepth != 0)
			return NULL;
//		if (header->numberofmipmaplevels != 1)
//			return false;	//only allow cubemaps that have no mips
	}
	else
		return NULL;	//don't allow weird cubemaps
//	if (header->pixeldepth && header->pixelwidth != header->pixeldepth && header->pixelheight != header->pixeldepth)
//		return NULL;	//we only support 3d textures where width+height+depth are the same. too lazy to change it now.

	/*FIXME: validate format+type for non-compressed formats*/
	switch(header.glinternalformat)
	{
	case 0x8D64/*GL_ETC1_RGB8_OES*/:							encoding = PTI_ETC1_RGB8;			break;
	case 0x9270/*GL_COMPRESSED_R11_EAC*/:						encoding = PTI_EAC_R11;				break;
	case 0x9271/*GL_COMPRESSED_SIGNED_R11_EAC*/:				encoding = PTI_EAC_R11_SNORM;		break;
	case 0x9272/*GL_COMPRESSED_RG11_EAC*/:						encoding = PTI_EAC_RG11;			break;
	case 0x9273/*GL_COMPRESSED_SIGNED_RG11_EAC*/:				encoding = PTI_EAC_RG11_SNORM;		break;
	case 0x9274/*GL_COMPRESSED_RGB8_ETC2*/:						encoding = PTI_ETC2_RGB8;			break;
	case 0x9275/*GL_COMPRESSED_SRGB8_ETC2*/:					encoding = PTI_ETC2_RGB8_SRGB;		break;
	case 0x9276/*GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2*/:	encoding = PTI_ETC2_RGB8A1;			break;
	case 0x9277/*GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2*/:encoding = PTI_ETC2_RGB8A1_SRGB;	break;
	case 0x9278/*GL_COMPRESSED_RGBA8_ETC2_EAC*/:				encoding = PTI_ETC2_RGB8A8;			break;
	case 0x9279/*GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC*/:			encoding = PTI_ETC2_RGB8A8_SRGB;	break;
	case 0x83F0/*GL_COMPRESSED_RGB_S3TC_DXT1_EXT*/:				encoding = PTI_BC1_RGB;				break;
	case 0x8C4C/*GL_COMPRESSED_SRGB_S3TC_DXT1_EXT*/:			encoding = PTI_BC1_RGB_SRGB;		break;
	case 0x83F1/*GL_COMPRESSED_RGBA_S3TC_DXT1_EXT*/:			encoding = PTI_BC1_RGBA;			break;
	case 0x8C4D/*GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT*/:		encoding = PTI_BC1_RGBA_SRGB;		break;
	case 0x83F2/*GL_COMPRESSED_RGBA_S3TC_DXT3_EXT*/:			encoding = PTI_BC2_RGBA;			break;
	case 0x8C4E/*GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT*/:		encoding = PTI_BC2_RGBA_SRGB;		break;
	case 0x83F3/*GL_COMPRESSED_RGBA_S3TC_DXT5_EXT*/:			encoding = PTI_BC3_RGBA;			break;
	case 0x8C4F/*GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT*/:		encoding = PTI_BC3_RGBA_SRGB;		break;
	case 0x8DBC/*GL_COMPRESSED_SIGNED_RED_RGTC1*/:				encoding = PTI_BC4_R_SNORM;			break;
	case 0x8DBB/*GL_COMPRESSED_RED_RGTC1*/:						encoding = PTI_BC4_R;				break;
	case 0x8DBE/*GL_COMPRESSED_SIGNED_RG_RGTC2*/:				encoding = PTI_BC5_RG_SNORM;		break;
	case 0x8DBD/*GL_COMPRESSED_RG_RGTC2*/:						encoding = PTI_BC5_RG;				break;
	case 0x8E8F/*GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB*/:	encoding = PTI_BC6_RGB_UFLOAT;		break;
	case 0x8E8E/*GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB*/:		encoding = PTI_BC6_RGB_SFLOAT;		break;
	case 0x8E8C/*GL_COMPRESSED_RGBA_BPTC_UNORM_ARB*/:			encoding = PTI_BC7_RGBA;			break;
	case 0x8E8D/*GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB*/:		encoding = PTI_BC7_RGBA_SRGB;		break;
	case 0x93B0/*GL_COMPRESSED_RGBA_ASTC_4x4_KHR*/:				encoding = PTI_ASTC_4X4_LDR;		break;
	case 0x93B1/*GL_COMPRESSED_RGBA_ASTC_5x4_KHR*/:				encoding = PTI_ASTC_5X4_LDR;		break;
	case 0x93B2/*GL_COMPRESSED_RGBA_ASTC_5x5_KHR*/:				encoding = PTI_ASTC_5X5_LDR;		break;
	case 0x93B3/*GL_COMPRESSED_RGBA_ASTC_6x5_KHR*/:				encoding = PTI_ASTC_6X5_LDR;		break;
	case 0x93B4/*GL_COMPRESSED_RGBA_ASTC_6x6_KHR*/:				encoding = PTI_ASTC_6X6_LDR;		break;
	case 0x93B5/*GL_COMPRESSED_RGBA_ASTC_8x5_KHR*/:				encoding = PTI_ASTC_8X5_LDR;		break;
	case 0x93B6/*GL_COMPRESSED_RGBA_ASTC_8x6_KHR*/:				encoding = PTI_ASTC_8X6_LDR;		break;
	case 0x93B7/*GL_COMPRESSED_RGBA_ASTC_8x8_KHR*/:				encoding = PTI_ASTC_8X8_LDR;		break;
	case 0x93B8/*GL_COMPRESSED_RGBA_ASTC_10x5_KHR*/:			encoding = PTI_ASTC_10X5_LDR;		break;
	case 0x93B9/*GL_COMPRESSED_RGBA_ASTC_10x6_KHR*/:			encoding = PTI_ASTC_10X6_LDR;		break;
	case 0x93BA/*GL_COMPRESSED_RGBA_ASTC_10x8_KHR*/:			encoding = PTI_ASTC_10X8_LDR;		break;
	case 0x93BB/*GL_COMPRESSED_RGBA_ASTC_10x10_KHR*/:			encoding = PTI_ASTC_10X10_LDR;		break;
	case 0x93BC/*GL_COMPRESSED_RGBA_ASTC_12x10_KHR*/:			encoding = PTI_ASTC_12X10_LDR;		break;
	case 0x93BD/*GL_COMPRESSED_RGBA_ASTC_12x12_KHR*/:			encoding = PTI_ASTC_12X12_LDR;		break;
	case 0x93D0/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR*/:		encoding = PTI_ASTC_4X4_SRGB;		break;
	case 0x93D1/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR*/:		encoding = PTI_ASTC_5X4_SRGB;		break;
	case 0x93D2/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR*/:		encoding = PTI_ASTC_5X5_SRGB;		break;
	case 0x93D3/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR*/:		encoding = PTI_ASTC_6X5_SRGB;		break;
	case 0x93D4/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR*/:		encoding = PTI_ASTC_6X6_SRGB;		break;
	case 0x93D5/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR*/:		encoding = PTI_ASTC_8X5_SRGB;		break;
	case 0x93D6/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR*/:		encoding = PTI_ASTC_8X6_SRGB;		break;
	case 0x93D7/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR*/:		encoding = PTI_ASTC_8X8_SRGB;		break;
	case 0x93D8/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR*/:	encoding = PTI_ASTC_10X5_SRGB;		break;
	case 0x93D9/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR*/:	encoding = PTI_ASTC_10X6_SRGB;		break;
	case 0x93DA/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR*/:	encoding = PTI_ASTC_10X8_SRGB;		break;
	case 0x93DB/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR*/:	encoding = PTI_ASTC_10X10_SRGB;		break;
	case 0x93DC/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR*/:	encoding = PTI_ASTC_12X10_SRGB;		break;
	case 0x93DD/*GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR*/:	encoding = PTI_ASTC_12X12_SRGB;		break;
	case 0x80E1/*GL_BGRA_EXT*/:									encoding = PTI_BGRA8;				break;	//not even an internal format
	case 0x1908/*GL_RGBA*/:
	case 0x8058/*GL_RGBA8*/:									encoding = (header.glformat==0x80E1/*GL_BGRA*/)?PTI_BGRA8:PTI_RGBA8;			break;	//unsized types shouldn't really be here
	case 0x805B/*GL_RGBA16*/:									encoding = PTI_RGBA16;				break;
	case 0x8C43/*GL_SRGB8_ALPHA8*/:								encoding = (header.glformat==0x80E1/*GL_BGRA*/)?PTI_BGRA8_SRGB:PTI_RGBA8_SRGB;	break;
	case 0x8040/*GL_LUMINANCE8*/:								encoding = PTI_L8;					break;
	case 0x8045/*GL_LUMINANCE8_ALPHA8*/:						encoding = PTI_L8A8;				break;
	case 0x881A/*GL_RGBA16F_ARB*/:								encoding = PTI_RGBA16F;				break;
	case 0x8815/*GL_RGB32F_ARB*/:								encoding = PTI_RGB32F;				break;
	case 0x8814/*GL_RGBA32F_ARB*/:								encoding = PTI_RGBA32F;				break;
	case 0x8059/*GL_RGB10_A2*/:									encoding = PTI_A2BGR10;				break;
	case 0x8229/*GL_R8*/:										encoding = PTI_R8;					break;
	case 0x822A/*GL_R16*/:										encoding = PTI_R16;					break;
	case 0x822B/*GL_RG8*/:										encoding = PTI_RG8;					break;
	case 0x8F94/*GL_R8_SNORM*/:									encoding = PTI_R8_SNORM;			break;
	case 0x8F95/*GL_RG8_SNORM*/:								encoding = PTI_RG8_SNORM;			break;
	case 0x81A5/*GL_DEPTH_COMPONENT16*/:						encoding = PTI_DEPTH16;				break;
	case 0x81A6/*GL_DEPTH_COMPONENT24*/:						encoding = PTI_DEPTH24;				break;
	case 0x81A7/*GL_DEPTH_COMPONENT32*/:						encoding = PTI_DEPTH32;				break;
	case 0x88F0/*GL_DEPTH24_STENCIL8*/:							encoding = PTI_DEPTH24_8;			break;
	case 0x822D/*GL_R16F*/:										encoding = PTI_R16F;				break;
	case 0x822E/*GL_R32F*/:										encoding = PTI_R32F;				break;

	case 0x8C40/*GL_SRGB*/:
	case 0x8C41/*GL_SRGB8*/:
		if (header.glformat==0x80E1/*GL_BGRA*/)
			encoding = PTI_BGRX8_SRGB;
		else if (header.glformat==0x1908/*GL_RGBA*/)
			encoding = PTI_RGBX8_SRGB;
		break;

	case 0x1907/*GL_RGB*/:	//invalid sized format. treat as GL_RGB8, and do weird checks.
	case 0x8051/*GL_RGB8*/:	//other sized RGB formats are treated based upon the data format rather than the sized format, in case they were meant to be converted by the driver...
	case 0x8C3D/*GL_RGB9_E5*/:
	case 0x8D62/*GL_RGB565*/:
	case 0x8C3A/*GL_R11F_G11F_B10F*/:
		if (header.glformat == 0x80E0/*GL_BGR*/)
			encoding = PTI_BGR8;
		else if (header.glformat == 0x80E1/*GL_BGRA*/)
			encoding = PTI_BGRX8;
		else if (header.glformat == 0x1907/*GL_RGB*/)
		{
			if (header.gltype == 0x8C3B/*GL_UNSIGNED_INT_10F_11F_11F_REV*/)
				encoding = PTI_B10G11R11F;
			else if (header.gltype == 0x8C3E/*GL_UNSIGNED_INT_5_9_9_9_REV*/)
				encoding = PTI_E5BGR9;
			else if (header.gltype == 0x8363/*GL_UNSIGNED_SHORT_5_6_5*/)
				encoding = PTI_RGB565;
			else
				encoding = PTI_RGB8;
		}
		else if (header.glformat == 0x1908/*GL_RGBA*/)
			encoding = PTI_RGBX8;
		else
			encoding = PTI_RGB8;
		break;
	case 0x8056/*GL_RGBA4*/:
	case 0x8057/*GL_RGB5_A1*/:
		if (header.glformat == 0x1908/*GL_RGBA*/ && header.gltype == 0x8034/*GL_UNSIGNED_SHORT_5_5_5_1*/)
			encoding = PTI_RGBA5551;
		else if (header.glformat == 0x80E1/*GL_BGRA*/ && header.gltype == 0x8366/*GL_UNSIGNED_SHORT_1_5_5_5_REV*/)
			encoding = PTI_ARGB1555;
		else if (header.glformat == 0x1908/*GL_RGBA*/ && header.gltype == 0x8033/*GL_UNSIGNED_SHORT_4_4_4_4*/)
			encoding = PTI_RGBA4444;
		else if (header.glformat == 0x80E1/*GL_BGRA*/ && header.gltype == 0x8365/*GL_UNSIGNED_SHORT_4_4_4_4_REV*/)
			encoding = PTI_ARGB4444;
		break;

	default:
		encoding = TF_INVALID;
		break;
	}
	if (encoding == TF_INVALID)
	{
		Con_Printf("Unsupported ktx internalformat %x in %s\n", header.glinternalformat, fname);
		return NULL;
	}

//	if (!sh_config.texfmt[encoding])
//	{
//		Con_Printf("KTX %s: encoding %x not supported on this system\n", fname, header->glinternalformat);
//		return false;
//	}

	mips = Z_Malloc(sizeof(*mips));
	mips->mipcount = 0;
	if (header.pixeldepth)
		mips->type = PTI_3D;
	else if (header.numberoffaces==6)
	{
		if (header.numberofarrayelements)
		{
			header.pixeldepth = header.numberofarrayelements*6;
			mips->type = PTI_CUBE_ARRAY;
		}
		else
			mips->type = PTI_CUBE;
	}
	else
	{
		if (header.numberofarrayelements)
		{
			header.pixeldepth = header.numberofarrayelements;
			mips->type = PTI_2D_ARRAY;
		}
		else
		{
			header.pixeldepth = 1;
			mips->type = PTI_2D;
		}
	}
	mips->extrafree = filedata;
	mips->encoding = encoding;

	filedata += sizeof(header);			//skip the header...
	filedata += header.bytesofkeyvaluedata;	//skip the keyvalue stuff

	if (nummips * header.numberoffaces > countof(mips->mip))
		nummips = countof(mips->mip) / header.numberoffaces;

	Image_BlockSizeForEncoding(encoding, &blockbytes, &blockwidth, &blockheight, &blockdepth);

	w = header.pixelwidth;
	h = max(1, header.pixelheight);
	d = max(1, header.pixeldepth);
	f = max(1, header.numberoffaces);
	l = max(1, header.numberofarrayelements);

	for (mipnum = 0; mipnum < nummips; mipnum++)
	{
		datasize = *(int*)filedata;
		filedata += 4;

		if (header.endianness == 0x01020304)
			datasize = LongSwap(datasize);

		browbytes = blockbytes * ((w+blockwidth-1)/blockwidth);
		padbytes = (browbytes & 3)?4-(browbytes&3):0;
		rows = ((h+blockheight-1)/blockheight)*
			   ((d+blockdepth-1)/blockdepth);
		if (datasize != (browbytes+padbytes) * rows)
		{
			Con_Printf("%s: mip %i does not match expected size (%u, required %u)\n", fname, mipnum, datasize, (browbytes+padbytes) * rows);
			break;
		}

		if (filedata + datasize*f*l > fileend)
		{
			Con_Printf("%s: truncation at mip %i\n", fname, mipnum);
			break;
		}

		if (mips->mipcount >= countof(mips->mip))
			break;
		mips->mip[mips->mipcount].width = w;
		mips->mip[mips->mipcount].height = h;
		mips->mip[mips->mipcount].depth = d*l*f;

		if (padbytes || header.endianness == 0x01020304)
		{	//gah.
			//the ktx format is 4-byte aligned. our internal representation is tightly packed (consistent with everything but gl).
			//in the case of byteswapping, any data types should work out okay (no misaligned stuff).
			rows *= l*f;
			mips->mip[mips->mipcount].needfree = true;
			mips->mip[mips->mipcount].datasize = browbytes * rows;
			mips->mip[mips->mipcount].data = BZ_Malloc(mips->mip[mips->mipcount].datasize);
			if (header.gltypesize == 4 && header.endianness == 0x01020304)
			{
				for (y = 0; y < rows; y++)
					for (x = 0; x < browbytes>>2; x++)
						((int*)((qbyte*)mips->mip[mips->mipcount].data + y*browbytes))[x] = LongSwap(((int*)filedata + y*browbytes+padbytes)[x]);
			}
			else if (header.gltypesize == 2 && header.endianness == 0x01020304)
			{
				for (y = 0; y < rows; y++)
					for (x = 0; x < browbytes>>1; x++)
						((short*)((qbyte*)mips->mip[mips->mipcount].data + y*browbytes))[x] = ShortSwap(((short*)filedata + y*browbytes+padbytes)[x]);
			}
			else
			{	//erk, panic...
				for (y = 0; y < rows; y++)
					memcpy((qbyte*)mips->mip[mips->mipcount].data + y*browbytes, filedata + y*browbytes+padbytes, browbytes);
			}
		}
		else
		{
			mips->mip[mips->mipcount].datasize = datasize * l*f;
			mips->mip[mips->mipcount].data = filedata;
		}
		mips->mipcount++;

		filedata += datasize *l*f;

		w = max(1, w>>1);
		h = max(1, h>>1);
		if (mips->type == PTI_3D)
			d = max(1, d>>1);
	}

	if (!mips->mipcount)
	{
		Z_Free(mips);
		return NULL;
	}

#ifdef ASTC_WITH_HDRTEST
	if (encoding >= PTI_ASTC_4X4_LDR && encoding < PTI_ASTC_4X4_SRGB)
	{
		int face;
		for (face = 0; face < header.numberoffaces; face++)
		{
			if (ASTC_BlocksAreHDR(mips->mip[face].data, mips->mip[face].datasize, blockwidth, blockheight, 1))
			{	//convert it to one of the hdr formats if we can.
				mips->encoding = PTI_ASTC_4X4_HDR+(encoding-PTI_ASTC_4X4_LDR);
				break;
			}
		}
	}
#endif

	return mips;
}

typedef struct
{
	char magic[12];
	quint32_t vkFormat;
	quint32_t typesize;
	quint32_t pixelwidth;
	quint32_t pixelheight;
	quint32_t pixeldepth;
	quint32_t layercount;
	quint32_t facecount;
	quint32_t levelcount;
	quint32_t compressionscheme;

	quint32_t dfdoffset;
	quint32_t dfdsize;
	quint32_t kvdoffset;
	quint32_t kvdsize;
	quint64_t sgdoffset;
	quint64_t sgdsize;
} ktx2header_t;
typedef struct
{
	quint64_t offset;
	quint64_t compsize;
	quint64_t rawsize;
} ktx2lavelheader_t;
static struct pendingtextureinfo *Image_ReadKTX2File(unsigned int flags, const char *fname, qbyte *filedata, size_t filesize)
{
	static const char magic[12] = {0xAB, 0x4B, 0x54, 0x58, 0x20, 0x32, 0x30, 0xBB, 0x0D, 0x0A, 0x1A, 0x0A};
	ktx2header_t header;
	const ktx2lavelheader_t *levelheader;
	int mipnum;
	unsigned int w, h, d;
	struct pendingtextureinfo *mips;
	int encoding = TF_INVALID, itype;

	unsigned int bw, bh, bd, bb;

	if (filesize < sizeof(ktxheader_t) || memcmp(filedata, magic, sizeof(magic)))
		return NULL;	//not a ktx file

	header = *(const ktx2header_t*)filedata;
	levelheader = (const ktx2lavelheader_t*)((const ktx2header_t*)filedata+1);

	header.vkFormat			= LittleLong(header.vkFormat);
	header.typesize			= LittleLong(header.typesize);
	header.pixelwidth		= LittleLong(header.pixelwidth);
	header.pixelheight		= LittleLong(header.pixelheight);
	header.pixeldepth		= LittleLong(header.pixeldepth);
	header.layercount		= LittleLong(header.layercount);
	header.facecount		= LittleLong(header.facecount);
	header.levelcount		= LittleLong(header.levelcount);
	header.compressionscheme= LittleLong(header.compressionscheme);
	header.dfdoffset		= LittleLong(header.dfdoffset);
	header.dfdsize			= LittleLong(header.dfdsize);
	header.kvdoffset		= LittleLong(header.kvdoffset);
	header.kvdsize			= LittleLong(header.kvdsize);
	header.sgdoffset		= LittleI64(header.sgdoffset);
	header.sgdsize			= LittleI64(header.sgdsize);

	if (!header.pixelheight)
		header.pixelheight = 1;	//we don't support 1D textures. force it to 2d.
	if (header.pixeldepth)
	{
		if (header.layercount || header.facecount!=1)
			return NULL;	//neither 3d arrays nor 3d cubes are supported, nor do they really make sense.
		header.layercount = 1;
		itype = PTI_3D;
	}
	else
	{
		header.pixeldepth = 1;

		if (header.facecount==6)
		{	//cube...
			if (header.layercount)
				itype = PTI_CUBE_ARRAY;
			else itype = PTI_CUBE, header.layercount=1;
		}
		else if (header.facecount == 1)
		{	//boring 2d
			if (header.layercount)
				itype = PTI_2D_ARRAY;
			else itype = PTI_2D, header.layercount=1;
		}
		else
			return NULL;	//not allowed
	}

	w = header.pixelwidth;
	h = header.pixelheight;
	d = header.pixeldepth*header.facecount*header.layercount;

	if (!header.levelcount)
		header.levelcount = 1;	//means we must auto-generate the mip pyramid. should warn if texflags doesn't match.

	switch (header.compressionscheme)
	{
	case 0:	//no compression
		break;
	case 1: //basis... w/e
	case 2:	//zstd... we have no decompression lib
	case 3: //zlib... we probably have zlib! but the docs imply zlib yet states raw deflate (so requires a passing the windowsize to zlib's inflateInit2 as negative, which is non-obvious).
	default:
		return NULL;	//we don't support this junk. gzip it. you'll get better compression than doing it per-level.
	}

	/*FIXME: validate format+type for non-compressed formats*/
	switch(header.vkFormat)
	{
//	case 1/*VK_FORMAT_R4G4_UNORM_PACK8*/:				encoding = PTI_RG4;			break;
	case 2/*VK_FORMAT_R4G4B4A4_UNORM_PACK16*/:			encoding = PTI_RGBA4444;	break;
//	case 3/*VK_FORMAT_B4G4R4A4_UNORM_PACK16*/:			encoding = PTI_BGRA4444;	break;
	case 4/*VK_FORMAT_R5G6B5_UNORM_PACK16*/:			encoding = PTI_RGB565;		break;
//	case 5/*VK_FORMAT_B5G6R5_UNORM_PACK16*/:			encoding = PTI_BGR565;		break;
	case 6/*VK_FORMAT_R5G5B5A1_UNORM_PACK16*/:			encoding = PTI_RGBA5551;	break;
//	case 7/*VK_FORMAT_B5G5R5A1_UNORM_PACK16*/:			encoding = PTI_BGRA5551;	break;
	case 8/*VK_FORMAT_A1R5G5B5_UNORM_PACK16*/:			encoding = PTI_ARGB1555;	break;
	case 9/*VK_FORMAT_R8_UNORM*/:						encoding = PTI_R8;			break;
	case 10/*VK_FORMAT_R8_SNORM*/:						encoding = PTI_R8_SNORM;	break;
//	case 11/*VK_FORMAT_R8_USCALED*/:
//	case 12/*VK_FORMAT_R8_SSCALED*/:
//	case 13/*VK_FORMAT_R8_UINT*/:
//	case 14/*VK_FORMAT_R8_SINT*/:
	case 15/*VK_FORMAT_R8_SRGB*/:						encoding = PTI_L8_SRGB;		break;	//erk
	case 16/*VK_FORMAT_R8G8_UNORM*/:					encoding = PTI_RG8;			break;
	case 17/*VK_FORMAT_R8G8_SNORM*/:					encoding = PTI_RG8_SNORM;	break;
//	case 18/*VK_FORMAT_R8G8_USCALED*/:
//	case 19/*VK_FORMAT_R8G8_SSCALED*/:
//	case 20/*VK_FORMAT_R8G8_UINT*/:
//	case 21/*VK_FORMAT_R8G8_SINT*/:
//	case 22/*VK_FORMAT_R8G8_SRGB*/:
	case 23/*VK_FORMAT_R8G8B8_UNORM*/:					encoding = PTI_RGB8;		break;
//	case 24/*VK_FORMAT_R8G8B8_SNORM*/:
//	case 25/*VK_FORMAT_R8G8B8_USCALED*/:
//	case 26/*VK_FORMAT_R8G8B8_SSCALED*/:
//	case 27/*VK_FORMAT_R8G8B8_UINT*/:
//	case 28/*VK_FORMAT_R8G8B8_SINT*/:
	case 29/*VK_FORMAT_R8G8B8_SRGB*/:					encoding = PTI_RGB8_SRGB;	break;
	case 30/*VK_FORMAT_B8G8R8_UNORM*/:					encoding = PTI_BGR8;		break;
//	case 31/*VK_FORMAT_B8G8R8_SNORM*/:
//	case 32/*VK_FORMAT_B8G8R8_USCALED*/:
//	case 33/*VK_FORMAT_B8G8R8_SSCALED*/:
//	case 34/*VK_FORMAT_B8G8R8_UINT*/:
//	case 35/*VK_FORMAT_B8G8R8_SINT*/:
	case 36/*VK_FORMAT_B8G8R8_SRGB*/:					encoding = PTI_BGR8_SRGB;	break;
	case 37/*VK_FORMAT_R8G8B8A8_UNORM*/:				encoding = PTI_RGBA8;		break;
//	case 38/*VK_FORMAT_R8G8B8A8_SNORM*/:
//	case 39/*VK_FORMAT_R8G8B8A8_USCALED*/:
//	case 40/*VK_FORMAT_R8G8B8A8_SSCALED*/:
//	case 41/*VK_FORMAT_R8G8B8A8_UINT*/:
//	case 42/*VK_FORMAT_R8G8B8A8_SINT*/:
	case 43/*VK_FORMAT_R8G8B8A8_SRGB*/:					encoding = PTI_RGBA8_SRGB;	break;
	case 44/*VK_FORMAT_B8G8R8A8_UNORM*/:				encoding = PTI_BGRA8;		break;
//	case 45/*VK_FORMAT_B8G8R8A8_SNORM*/:
//	case 46/*VK_FORMAT_B8G8R8A8_USCALED*/:
//	case 47/*VK_FORMAT_B8G8R8A8_SSCALED*/:
//	case 48/*VK_FORMAT_B8G8R8A8_UINT*/:
//	case 49/*VK_FORMAT_B8G8R8A8_SINT*/:
	case 50/*VK_FORMAT_B8G8R8A8_SRGB*/:					encoding = PTI_BGRA8_SRGB;	break;
//	case 51/*VK_FORMAT_A8B8G8R8_UNORM_PACK32*/:
//	case 52/*VK_FORMAT_A8B8G8R8_SNORM_PACK32*/:
//	case 53/*VK_FORMAT_A8B8G8R8_USCALED_PACK32*/:
//	case 54/*VK_FORMAT_A8B8G8R8_SSCALED_PACK32*/:
//	case 55/*VK_FORMAT_A8B8G8R8_UINT_PACK32*/:
//	case 56/*VK_FORMAT_A8B8G8R8_SINT_PACK32*/:
//	case 57/*VK_FORMAT_A8B8G8R8_SRGB_PACK32*/:
//	case 58/*VK_FORMAT_A2R10G10B10_UNORM_PACK32*/:
//	case 59/*VK_FORMAT_A2R10G10B10_SNORM_PACK32*/:
//	case 60/*VK_FORMAT_A2R10G10B10_USCALED_PACK32*/:
//	case 61/*VK_FORMAT_A2R10G10B10_SSCALED_PACK32*/:
//	case 62/*VK_FORMAT_A2R10G10B10_UINT_PACK32*/:
//	case 63/*VK_FORMAT_A2R10G10B10_SINT_PACK32*/:
	case 64/*VK_FORMAT_A2B10G10R10_UNORM_PACK32*/:		encoding = PTI_A2BGR10;		break;
//	case 65/*VK_FORMAT_A2B10G10R10_SNORM_PACK32*/:
//	case 66/*VK_FORMAT_A2B10G10R10_USCALED_PACK32*/:
//	case 67/*VK_FORMAT_A2B10G10R10_SSCALED_PACK32*/:
//	case 68/*VK_FORMAT_A2B10G10R10_UINT_PACK32*/:
//	case 69/*VK_FORMAT_A2B10G10R10_SINT_PACK32*/:
	case 70/*VK_FORMAT_R16_UNORM*/:						encoding = PTI_R16;			break;
//	case 71/*VK_FORMAT_R16_SNORM*/:
//	case 72/*VK_FORMAT_R16_USCALED*/:
//	case 73/*VK_FORMAT_R16_SSCALED*/:
//	case 74/*VK_FORMAT_R16_UINT*/:
//	case 75/*VK_FORMAT_R16_SINT*/:
	case 76/*VK_FORMAT_R16_SFLOAT*/:					encoding = PTI_R16F;		break;
//	case 77/*VK_FORMAT_R16G16_UNORM*/:
//	case 78/*VK_FORMAT_R16G16_SNORM*/:
//	case 79/*VK_FORMAT_R16G16_USCALED*/:
//	case 80/*VK_FORMAT_R16G16_SSCALED*/:
//	case 81/*VK_FORMAT_R16G16_UINT*/:
//	case 82/*VK_FORMAT_R16G16_SINT*/:
//	case 83/*VK_FORMAT_R16G16_SFLOAT*/:
//	case 84/*VK_FORMAT_R16G16B16_UNORM*/:
//	case 85/*VK_FORMAT_R16G16B16_SNORM*/:
//	case 86/*VK_FORMAT_R16G16B16_USCALED*/:
//	case 87/*VK_FORMAT_R16G16B16_SSCALED*/:
//	case 88/*VK_FORMAT_R16G16B16_UINT*/:
//	case 89/*VK_FORMAT_R16G16B16_SINT*/:
	case 90/*VK_FORMAT_R16G16B16_SFLOAT*/:				encoding = PTI_RGB32F;		break;
	case 91/*VK_FORMAT_R16G16B16A16_UNORM*/:			encoding = PTI_RGBA16;		break;
//	case 92/*VK_FORMAT_R16G16B16A16_SNORM*/:
//	case 93/*VK_FORMAT_R16G16B16A16_USCALED*/:
//	case 94/*VK_FORMAT_R16G16B16A16_SSCALED*/:
//	case 95/*VK_FORMAT_R16G16B16A16_UINT*/:
//	case 96/*VK_FORMAT_R16G16B16A16_SINT*/:
	case 97/*VK_FORMAT_R16G16B16A16_SFLOAT*/:			encoding = PTI_RGBA16F;		break;
//	case 98/*VK_FORMAT_R32_UINT*/:
//	case 99/*VK_FORMAT_R32_SINT*/:
	case 100/*VK_FORMAT_R32_SFLOAT*/:					encoding = PTI_R32F;		break;
//	case 101/*VK_FORMAT_R32G32_UINT*/:
//	case 102/*VK_FORMAT_R32G32_SINT*/:
//	case 103/*VK_FORMAT_R32G32_SFLOAT*/:
//	case 104/*VK_FORMAT_R32G32B32_UINT*/:
//	case 105/*VK_FORMAT_R32G32B32_SINT*/:
	case 106/*VK_FORMAT_R32G32B32_SFLOAT*/:				encoding = PTI_RGB32F;		break;
//	case 107/*VK_FORMAT_R32G32B32A32_UINT*/:
//	case 108/*VK_FORMAT_R32G32B32A32_SINT*/:
	case 109/*VK_FORMAT_R32G32B32A32_SFLOAT*/:			encoding = PTI_RGBA32F;		break;
//	case 110/*VK_FORMAT_R64_UINT*/:
//	case 111/*VK_FORMAT_R64_SINT*/:
//	case 112/*VK_FORMAT_R64_SFLOAT*/:
//	case 113/*VK_FORMAT_R64G64_UINT*/:
//	case 114/*VK_FORMAT_R64G64_SINT*/:
//	case 115/*VK_FORMAT_R64G64_SFLOAT*/:
//	case 116/*VK_FORMAT_R64G64B64_UINT*/:
//	case 117/*VK_FORMAT_R64G64B64_SINT*/:
//	case 118/*VK_FORMAT_R64G64B64_SFLOAT*/:
//	case 119/*VK_FORMAT_R64G64B64A64_UINT*/:
//	case 120/*VK_FORMAT_R64G64B64A64_SINT*/:
//	case 121/*VK_FORMAT_R64G64B64A64_SFLOAT*/:
	case 122/*VK_FORMAT_B10G11R11_UFLOAT_PACK32*/:		encoding = PTI_B10G11R11F;	break;
	case 123/*VK_FORMAT_E5B9G9R9_UFLOAT_PACK32*/:		encoding = PTI_E5BGR9;		break;
	//case 124/*VK_FORMAT_D16_UNORM*/:					encoding = PTI_DEPTH16;		break;
	//case 125/*VK_FORMAT_X8_D24_UNORM_PACK32*/:		encoding = PTI_DEPTH24;		break;
	//case 126/*VK_FORMAT_D32_SFLOAT*/:					encoding = PTI_DEPTH32;		break;
//	case 127/*VK_FORMAT_S8_UINT*/:
//	case 128/*VK_FORMAT_D16_UNORM_S8_UINT*/:
	//case 129/*VK_FORMAT_D24_UNORM_S8_UINT*/:			encoding = PTI_DEPTH24_8;	break;
//	case 130/*VK_FORMAT_D32_SFLOAT_S8_UINT*/:
	case 131/*VK_FORMAT_BC1_RGB_UNORM_BLOCK*/:			encoding = PTI_BC1_RGB;			break;
	case 132/*VK_FORMAT_BC1_RGB_SRGB_BLOCK*/:			encoding = PTI_BC1_RGB_SRGB;	break;
	case 133/*VK_FORMAT_BC1_RGBA_UNORM_BLOCK*/:			encoding = PTI_BC1_RGBA;		break;
	case 134/*VK_FORMAT_BC1_RGBA_SRGB_BLOCK*/:			encoding = PTI_BC1_RGBA_SRGB;	break;
	case 135/*VK_FORMAT_BC2_UNORM_BLOCK*/:				encoding = PTI_BC2_RGBA;		break;
	case 136/*VK_FORMAT_BC2_SRGB_BLOCK*/:				encoding = PTI_BC2_RGBA_SRGB;	break;
	case 137/*VK_FORMAT_BC3_UNORM_BLOCK*/:				encoding = PTI_BC3_RGBA;		break;
	case 138/*VK_FORMAT_BC3_SRGB_BLOCK*/:				encoding = PTI_BC1_RGBA_SRGB;	break;
	case 139/*VK_FORMAT_BC4_UNORM_BLOCK*/:				encoding = PTI_BC4_R;			break;
	case 140/*VK_FORMAT_BC4_SNORM_BLOCK*/:				encoding = PTI_BC4_R_SNORM;		break;
	case 141/*VK_FORMAT_BC5_UNORM_BLOCK*/:				encoding = PTI_BC5_RG;			break;
	case 142/*VK_FORMAT_BC5_SNORM_BLOCK*/:				encoding = PTI_BC5_RG_SNORM;	break;
	case 143/*VK_FORMAT_BC6H_UFLOAT_BLOCK*/:			encoding = PTI_BC6_RGB_UFLOAT;	break;
	case 144/*VK_FORMAT_BC6H_SFLOAT_BLOCK*/:			encoding = PTI_BC6_RGB_SFLOAT;	break;
	case 145/*VK_FORMAT_BC7_UNORM_BLOCK*/:				encoding = PTI_BC7_RGBA;		break;
	case 146/*VK_FORMAT_BC7_SRGB_BLOCK*/:				encoding = PTI_BC7_RGBA_SRGB;	break;
	case 147/*VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK*/:		encoding = PTI_ETC2_RGB8;		break;
	case 148/*VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK*/:		encoding = PTI_ETC2_RGB8_SRGB;	break;
	case 149/*VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK*/:	encoding = PTI_ETC2_RGB8A1;		break;
	case 150/*VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK*/:		encoding = PTI_ETC2_RGB8A1_SRGB;break;
	case 151/*VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK*/:	encoding = PTI_ETC2_RGB8A8;		break;
	case 152/*VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK*/:		encoding = PTI_ETC2_RGB8A8_SRGB;break;
	case 153/*VK_FORMAT_EAC_R11_UNORM_BLOCK*/:			encoding = PTI_EAC_R11;			break;
	case 154/*VK_FORMAT_EAC_R11_SNORM_BLOCK*/:			encoding = PTI_EAC_R11_SNORM;	break;
	case 155/*VK_FORMAT_EAC_R11G11_UNORM_BLOCK*/:		encoding = PTI_EAC_RG11;		break;
	case 156/*VK_FORMAT_EAC_R11G11_SNORM_BLOCK*/:		encoding = PTI_EAC_RG11_SNORM;	break;
	case 157/*VK_FORMAT_ASTC_4x4_UNORM_BLOCK*/:			encoding = PTI_ASTC_4X4_LDR;	break;
	case 158/*VK_FORMAT_ASTC_4x4_SRGB_BLOCK*/:			encoding = PTI_ASTC_4X4_SRGB;	break;
	case 159/*VK_FORMAT_ASTC_5x4_UNORM_BLOCK*/:			encoding = PTI_ASTC_5X4_LDR;	break;
	case 160/*VK_FORMAT_ASTC_5x4_SRGB_BLOCK*/:			encoding = PTI_ASTC_5X4_SRGB;	break;
	case 161/*VK_FORMAT_ASTC_5x5_UNORM_BLOCK*/:			encoding = PTI_ASTC_5X5_LDR;	break;
	case 162/*VK_FORMAT_ASTC_5x5_SRGB_BLOCK*/:			encoding = PTI_ASTC_5X5_SRGB;	break;
	case 163/*VK_FORMAT_ASTC_6x5_UNORM_BLOCK*/:			encoding = PTI_ASTC_6X5_LDR;	break;
	case 164/*VK_FORMAT_ASTC_6x5_SRGB_BLOCK*/:			encoding = PTI_ASTC_6X5_SRGB;	break;
	case 165/*VK_FORMAT_ASTC_6x6_UNORM_BLOCK*/:			encoding = PTI_ASTC_6X6_LDR;	break;
	case 166/*VK_FORMAT_ASTC_6x6_SRGB_BLOCK*/:			encoding = PTI_ASTC_6X6_SRGB;	break;
	case 167/*VK_FORMAT_ASTC_8x5_UNORM_BLOCK*/:			encoding = PTI_ASTC_8X5_LDR;	break;
	case 168/*VK_FORMAT_ASTC_8x5_SRGB_BLOCK*/:			encoding = PTI_ASTC_8X5_SRGB;	break;
	case 169/*VK_FORMAT_ASTC_8x6_UNORM_BLOCK*/:			encoding = PTI_ASTC_8X6_LDR;	break;
	case 170/*VK_FORMAT_ASTC_8x6_SRGB_BLOCK*/:			encoding = PTI_ASTC_8X6_SRGB;	break;
	case 171/*VK_FORMAT_ASTC_8x8_UNORM_BLOCK*/:			encoding = PTI_ASTC_8X8_LDR;	break;
	case 172/*VK_FORMAT_ASTC_8x8_SRGB_BLOCK*/:			encoding = PTI_ASTC_8X8_SRGB;	break;
	case 173/*VK_FORMAT_ASTC_10x5_UNORM_BLOCK*/:		encoding = PTI_ASTC_10X5_LDR;	break;
	case 174/*VK_FORMAT_ASTC_10x5_SRGB_BLOCK*/:			encoding = PTI_ASTC_10X5_SRGB;	break;
	case 175/*VK_FORMAT_ASTC_10x6_UNORM_BLOCK*/:		encoding = PTI_ASTC_10X6_LDR;	break;
	case 176/*VK_FORMAT_ASTC_10x6_SRGB_BLOCK*/:			encoding = PTI_ASTC_10X6_SRGB;	break;
	case 177/*VK_FORMAT_ASTC_10x8_UNORM_BLOCK*/:		encoding = PTI_ASTC_10X8_LDR;	break;
	case 178/*VK_FORMAT_ASTC_10x8_SRGB_BLOCK*/:			encoding = PTI_ASTC_10X8_SRGB;	break;
	case 179/*VK_FORMAT_ASTC_10x10_UNORM_BLOCK*/:		encoding = PTI_ASTC_10X10_LDR;	break;
	case 180/*VK_FORMAT_ASTC_10x10_SRGB_BLOCK*/:		encoding = PTI_ASTC_10X10_SRGB;	break;
	case 181/*VK_FORMAT_ASTC_12x10_UNORM_BLOCK*/:		encoding = PTI_ASTC_12X10_LDR;	break;
	case 182/*VK_FORMAT_ASTC_12x10_SRGB_BLOCK*/:		encoding = PTI_ASTC_12X10_SRGB;	break;
	case 183/*VK_FORMAT_ASTC_12x12_UNORM_BLOCK*/:		encoding = PTI_ASTC_12X12_LDR;	break;
	case 184/*VK_FORMAT_ASTC_12x12_SRGB_BLOCK*/:		encoding = PTI_ASTC_12X12_SRGB;	break;

	case 0/*VK_FORMAT_UNDEFINED*/:
	default:
		encoding = PTI_INVALID;
		break;
	}
	if (encoding == PTI_INVALID)
	{
		//TODO: we might be able to make sense of these by decoding the DFD.
		Con_Printf(CON_WARNING"%s: Unsupported ktx2 vkformat %x\n", fname, header.vkFormat);
		return NULL;
	}

	if (header.kvdsize)
	{
		size_t kvd = header.kvdoffset;
		const qbyte *key;	//utf-8.
		const qbyte *val;	//often utf-8, but might be binary.
		size_t kvdend = kvd+header.kvdsize;
//		VALGRIND_MAKE_MEM_UNDEFINED(filedata+kvdend, 1);
		while(kvd+4 <= kvdend)
		{
			quint32_t len = (filedata[kvd+0]<<0)|(filedata[kvd+1]<<8)|(filedata[kvd+2]<<16)|(filedata[kvd+3]<<24), klen;
			quint32_t vlen;
			kvd+=4;
			if (kvd+len > kvdend)
				break;	//some sort of error
			for(klen = 0;;)
			{
				if (!filedata[kvd+klen++])
					break;
				if (klen >= len)
				{
					Con_Printf(CON_WARNING"%s: unterminated kvd key\n", fname);
					return NULL; //we NEED a null for it to be valid.
				}
			}
			key = filedata+kvd;
			val = filedata+kvd+klen;
			vlen = len-klen;
			if (!strcmp(key, "KTXwriter"))
				;
			else if (!strcmp(key, "KTXcubemapIncomplete"))
			{
				Con_Printf(CON_WARNING"%s: incomplete cubemaps are not supported\n", fname);
				return NULL;	//would be seen as a 2darray.
			}
			else if (!strcmp(key, "KTXorientation"))
			{
				if (vlen >= 2 && val[0] && val[1] == 'u')
					Con_Printf("%s: warning: image is bottom up\n", fname);
			}
			else if (!strcmp(key, "KTXglFormat"))
				/*uninteresting, we're not loading it directly*/;
			else if (!strcmp(key, "KTXdxgiFormat__"))	//why do the docs say the trailing underscores?
				/*uninteresting, we're not loading it directly*/;
			else if (!strcmp(key, "KTXmetalPixelFormat"))
				/*uninteresting, we're not loading it directly*/;
			else if (!strcmp(key, "KTXswizzle"))
			{
				if (encoding == PTI_R8 && vlen >= 5 && !strcmp(val, "rrr1"))
					encoding = PTI_L8;
//				else if (encoding == PTI_R8_SRGB && vlen >= 5 && !strcmp(val, "rrr1"))
//					encoding = PTI_L8_SRGB;
				else if (encoding == PTI_RG8 && vlen >= 5 && !strcmp(val, "rrrg"))
					encoding = PTI_L8A8;
				else
					Con_Printf("%s: unsupported swizzle: %s\n", fname, val);
			}
			else if (!strcmp(key, "KTXwriterScParams"))
				;
			else if (!strcmp(key, "KTXastcDecodeMode"))
				;
			else if (!strcmp(key, "KTXanimData"))
				/*uint32_t duration, timescale, loopcount*/;
			else
				Con_Printf("%s: unhandled kvd: %s\n", fname, key);
			kvd+=(len+3)&~3;	//padding... strangely the start offset does not 'need' to be aligned. weird.
		}
		if (kvd != kvdend)
			Con_Printf(CON_WARNING"%s: misparsed kvd data\n", fname);
//		VALGRIND_MAKE_MEM_DEFINED_IF_ADDRESSABLE(filedata+kvdend, 1);
	}

	Image_BlockSizeForEncoding(encoding, &bb, &bw, &bh, &bd);

	mips = Z_Malloc(sizeof(*mips));
	mips->encoding = encoding;
	mips->type = itype;
	mips->mipcount = 0;
	mips->extrafree = filedata;
	mips->encoding = encoding;

	if (header.levelcount > countof(mips->mip))
		header.levelcount = countof(mips->mip);

	for (mipnum = 0; mipnum < header.levelcount; mipnum++, levelheader++)
	{
		size_t ofs = LittleI64(levelheader->offset);
		qbyte *src = filedata + ofs;
		size_t csz = LittleI64(levelheader->compsize);
		size_t rsz = LittleI64(levelheader->rawsize);
		size_t needsize =	((w+bw-1)/bw)*
							((h+bh-1)/bh)*
							((d+bd-1)/bd)*
							bb;
		if (rsz != needsize)
		{
			Con_Printf(CON_WARNING"%s: mip %i does not match expected size (%u, required %u)\n", fname, mipnum, (unsigned int)rsz, (unsigned int)needsize);
			break;
		}
		if (ofs+csz > filesize || csz > filesize || ofs+csz < ofs)
		{
			Con_Printf(CON_WARNING"%s: truncation at mip %i\n", fname, mipnum);
			break;
		}
		if (rsz != csz)
		{
			Con_Printf(CON_WARNING"%s: compression size mismatch\n", fname);
			break;
		}

		if (mips->mipcount >= countof(mips->mip))
			break;

		mips->mip[mips->mipcount].width = w;
		mips->mip[mips->mipcount].height = h;
		mips->mip[mips->mipcount].depth = d;
		mips->mip[mips->mipcount].datasize = rsz;
		mips->mip[mips->mipcount].data = src;
#ifndef FTE_LITTLE_ENDIAN
		switch(header.typesize)
		{
		case 1:
			break;
		case 4:
			{
				quint32_t *in = mips->mip[mips->mipcount].data;
				quint32_t *out = mips->mip[mips->mipcount].data = BZ_Malloc(rsz);
				mips->mip[mips->mipcount].needfree = true;
				for (csz = 0; csz < rsz; csz+=4)
					*out++ = LittleLong(*in++);
			}
			break;
		case 2:
			{
				quint16_t *in = mips->mip[mips->mipcount].data;
				quint16_t *out = mips->mip[mips->mipcount].data = BZ_Malloc(rsz);
				mips->mip[mips->mipcount].needfree = true;
				for (csz = 0; csz < rsz; csz+=2)
					*out++ = LittleShort(*in++);
			}
			break;
		default:
			Con_Printf(CON_WARNING"%s: unsupported type size.\n", fname);
			Z_Free(mips);
			return NULL;
		}
#endif
		mips->mipcount++;

		w = max(1, w>>1);
		h = max(1, h>>1);
		if (mips->type == PTI_3D)
			d = max(1, d>>1);
	}

	if (!mips->mipcount)
	{
		Z_Free(mips);
		return NULL;
	}

#ifdef ASTC_WITH_HDRTEST
	if (encoding >= PTI_ASTC_4X4_LDR && encoding < PTI_ASTC_4X4_SRGB)
	{	//assumption: if any levels are hdr then level0 will contain such a block. might be nicer to start mid-way though, for less blocks.
		if (ASTC_BlocksAreHDR(mips->mip[0].data, mips->mip[0].datasize, bw, bh, bd))
		{	//convert it to one of the hdr formats if we can.
			mips->encoding = PTI_ASTC_4X4_HDR+(encoding-PTI_ASTC_4X4_LDR);
		}
	}
#endif

	return mips;
}
struct pendingtextureinfo *Image_ReadKTXFile(unsigned int flags, const char *fname, qbyte *filedata, size_t filesize)
{
	if (filesize >= 12)
	{
		if (filedata[0] == 0xAB && filedata[1] == 0x4B && filedata[2] == 0x54 && filedata[3] == 0x58 && filedata[4] == 0x20)
		{
			if (filedata[5] == '2')
				return Image_ReadKTX2File(flags, fname, filedata, filesize);
			else
				return Image_ReadKTX1File(flags, fname, filedata, filesize);
		}
	}
	return NULL;	//not enough size for the header.
}

#endif // IMAGEFMT_KTX
