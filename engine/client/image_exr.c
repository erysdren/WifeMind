#include "image.h"

#ifdef IMAGEFMT_EXR

#if 0
	#include "OpenEXR/ImfCRgbaFile.h"
#else
	typedef void ImfInputFile;
	typedef void ImfHeader;
	typedef void ImfRgba;
#endif
static struct
{
	void *handle;
	ImfInputFile   *(*OpenInputFile)		(const char name[]);
	int				(*CloseInputFile)		(ImfInputFile *in);
	int				(*InputSetFrameBuffer)	(ImfInputFile *in, ImfRgba *base, size_t xStride, size_t yStride);
	int				(*InputReadPixels)		(ImfInputFile *in, int scanLine1, int scanLine2);
	const ImfHeader*(*InputHeader)			(const ImfInputFile *in);

	void			(*HeaderDataWindow) (const ImfHeader *hdr, int *xMin, int *yMin, int *xMax, int *yMax);
} exr;
void InitLibrary_OpenEXR(void)
{
#ifdef IMF_MAGIC
	exr.OpenInputFile		= ImfOpenInputFile;
	exr.CloseInputFile		= ImfCloseInputFile;
	exr.InputSetFrameBuffer	= ImfInputSetFrameBuffer;
	exr.InputReadPixels		= ImfInputReadPixels;
	exr.InputHeader			= ImfInputHeader;
	exr.HeaderDataWindow	= ImfHeaderDataWindow;
#else
	dllfunction_t funcs[] =
	{
		{(void**)&exr.OpenInputFile,		"ImfOpenInputFile"},
		{(void**)&exr.CloseInputFile,		"ImfCloseInputFile"},
		{(void**)&exr.InputSetFrameBuffer,	"ImfInputSetFrameBuffer"},
		{(void**)&exr.InputReadPixels,		"ImfInputReadPixels"},
		{(void**)&exr.InputHeader,			"ImfInputHeader"},
		{(void**)&exr.HeaderDataWindow,		"ImfHeaderDataWindow"},
		{NULL}
	};
	#ifdef __linux__
		//its some shitty c++ library, so while the C api that we use is stable, nothing else is so it has lots of random names.
		if (!exr.handle)
			exr.handle = Sys_LoadLibrary("libIlmImf-2_3.so.24", funcs);	//debian sid(bullseye)
		if (!exr.handle)
			exr.handle = Sys_LoadLibrary("libIlmImf-2_2.so.23", funcs);	//debian buster
		if (!exr.handle)
			exr.handle = Sys_LoadLibrary("libIlmImf-2_2.so.22", funcs);	//debian stretch
	#endif

	//try the generic/dev name.
	if (!exr.handle)
		exr.handle = Sys_LoadLibrary("libIlmImf", funcs);
#endif
}
qboolean OpenEXR_IsLibraryLoaded(void)
{
	return !!exr.handle;
}
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif
void *ReadEXRFile(qbyte *buf, size_t len, const char *fname, int *outwidth, int *outheight, uploadfmt_t *outformat)
{
	char tname[] = "/tmp/exr.XXXXXX";
#ifndef _WIN32
	int fd;
#endif
	ImfInputFile *ctx;
	const ImfHeader *hdr;
	void *result;

	if (!exr.handle)
	{
		Con_Printf("%s: libIlmImf not loaded\n", fname);
		return NULL;
	}

	//shitty API that only supports filenames, so now that we've read it from a file, write it to a file so that it can be read. See, shitty.
#ifdef _WIN32
	if (!_mktemp(tname))
	{
		DWORD sizewritten;
		HANDLE fd = CreateFileA(tname, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE, NULL);
		WriteFile(fd, buf, len, &sizewritten, NULL); //assume we managed to write it all
		ctx = exr.OpenInputFile(tname); //is this be ansi or oem or utf-16 or what? lets assume that it has no idea either.
		CloseHandle(fd);
#else
	fd = mkstemp(tname);	//bsd4.3/posix1-2001
	if (fd >= 0)
	{
		if (write(fd, buf, len) == len)
			ctx = exr.OpenInputFile(tname);
		else
			ctx = NULL;
		close(fd);	//we don't need the input file now.
		unlink(tname);
#endif

		if (ctx)
		{
			int xmin, xmax, ymin, ymax;
			hdr = exr.InputHeader(ctx);
			exr.HeaderDataWindow(hdr, &xmin,&ymin, &xmax,&ymax);
			*outwidth = (xmax-xmin)+1;
			*outheight = (ymax-ymin)+1;
			result = BZ_Malloc(sizeof(short)*4u*(size_t)*outwidth**outheight);
			exr.InputSetFrameBuffer(ctx, (char*)result-xmin*8-ymin*(size_t)*outwidth*8, 1, *outwidth);
			exr.InputReadPixels(ctx, ymin, ymax);
			exr.CloseInputFile(ctx);
			*outformat = PTI_RGBA16F;	//output is always half-floats.
			return result;
		}
	}
	return NULL;
}

#endif // IMAGEFMT_EXR
