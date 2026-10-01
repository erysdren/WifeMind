#ifndef _IMAGE_H_
#define _IMAGE_H_

#include "quakedef.h"
#include "shader.h"
#include "glquake.h"

// image.c
float HalfToFloat(unsigned short val);
unsigned short FloatToHalf(float val);

#ifdef IMAGEFMT_BLP
// image_blp.c
struct pendingtextureinfo *Image_ReadBLPFile(unsigned int flags, const char *fname, qbyte *filedata, size_t filesize);
#endif

#ifdef IMAGEFMT_BMP
// image_bmp.c
qbyte *ReadBMPFile(qbyte *buf, int length, int *width, int *height);
qboolean WriteBMPFile(char *filename, enum fs_relative fsroot, qbyte *in, qintptr_t instride, int width, int height, uploadfmt_t fmt);
qbyte *ReadICOFile(const char *fname, qbyte *buf, int length, int *width, int *height, uploadfmt_t *fmt);
#endif

#ifdef IMAGEFMT_DDS
// image_dds.c
qboolean Image_WriteDDSFile(const char *filename, enum fs_relative fsroot, struct pendingtextureinfo *mips);
struct pendingtextureinfo *Image_ReadDDSFile(unsigned int flags, const char *fname, qbyte *filedata, size_t filesize);
#endif

#ifdef IMAGEFMT_EXR
// image_exr.c
void InitLibrary_OpenEXR(void);
qboolean OpenEXR_IsLibraryLoaded(void);
void *ReadEXRFile(qbyte *buf, size_t len, const char *fname, int *outwidth, int *outheight, uploadfmt_t *outformat);
#endif

#ifdef IMAGEFMT_KTX
// image_ktx.c
qboolean Image_WriteKTXFile(const char *filename, enum fs_relative fsroot, struct pendingtextureinfo *mips);
struct pendingtextureinfo *Image_ReadKTXFile(unsigned int flags, const char *fname, qbyte *filedata, size_t filesize);
#endif

#ifdef IMAGEFMT_HDR
// image_hdr.c
void *ReadRadianceFile(qbyte *buf, size_t len, const char *fname, int *width, int *height, uploadfmt_t *format);
#endif

#ifdef IMAGEFMT_PBM
// image_pbm.c
qbyte *ReadPBMFile(qbyte *buf, size_t len, const char *fname, int *width, int *height, uploadfmt_t *format);
#endif

#ifdef IMAGEFMT_PCX
// image_pcx.c
qboolean WritePCXfile(const char *filename, enum fs_relative fsroot, qbyte *data, int width, int height, int rowbytes, qbyte *palette, qboolean upload);
qbyte *ReadPCXFile(qbyte *buf, int length, int *width, int *height);
#endif

#ifdef IMAGEFMT_PSD
// image_psd.c
void *ReadPSDFile(qbyte *buf, size_t len, const char *fname, int *outwidth, int *outheight, uploadfmt_t *outformat);
#endif

#ifdef IMAGEFMT_PVR
// image_pvr.c
qbyte *ReadPVRFile(qbyte *buf, int len, int *width, int *height, uploadfmt_t *format, qboolean force_rgba8);
#endif

#ifdef IMAGEFMT_XCF
// image_xcf.c
qbyte *ReadXCFFile(const qbyte *filedata, size_t len, const char *fname, int *width, int *height, uploadfmt_t *format);
#endif

#ifdef IMAGEFMT_TGA
// image_tga.c
qboolean WriteTGA(const char *filename, enum fs_relative fsroot, const qbyte *fte_restrict rgb_buffer, qintptr_t bytestride, int width, int height, enum uploadfmt fmt);
void *ReadTargaFile(qbyte *buf, int length, int *width, int *height, uploadfmt_t *format, qboolean greyonly, uploadfmt_t forceformat);
#endif

#endif // _IMAGE_H_
