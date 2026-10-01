#ifndef _IMAGE_H_
#define _IMAGE_H_

#include "quakedef.h"
#include "shader.h"
#include "glquake.h"

#ifdef IMAGEFMT_BMP
// image_bmp.c
qbyte *ReadBMPFile(qbyte *buf, int length, int *width, int *height);
qboolean WriteBMPFile(char *filename, enum fs_relative fsroot, qbyte *in, qintptr_t instride, int width, int height, uploadfmt_t fmt);
qbyte *ReadICOFile(const char *fname, qbyte *buf, int length, int *width, int *height, uploadfmt_t *fmt);
#endif

#ifdef IMAGEFMT_PVR
// image_pvr.c
qbyte *ReadPVRFile(qbyte *buf, int len, int *width, int *height, uploadfmt_t *format, qboolean force_rgba8);
#endif

#endif // _IMAGE_H_
