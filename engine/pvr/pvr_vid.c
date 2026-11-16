#include "quakedef.h"
#ifdef PVRQUAKE
#include "pvr.h"

static unsigned int framenumber = 0;

qboolean PVR_VID_Init(rendererstate_t *info, unsigned char *palette)
{
	pvr_init_defaults();
	pvr_set_bg_color(0.0f, 0.5f, 0.5f);

	return true;
}

void PVR_VID_DeInit(void)
{
	pvr_shutdown();
}

qboolean PVR_VID_ApplyGammaRamps(unsigned int rampcount, unsigned short *ramps)
{
	return false;
}

char *PVR_VID_GetRGBInfo(int *bytestride, int *truevidwidth, int *truevidheight, enum uploadfmt *fmt)
{
	return NULL;
}

void PVR_VID_SetWindowCaption(const char *msg)
{

}

void PVR_VID_SwapBuffers(void)
{
	pvr_scene_finish();
	framenumber++;
}

void PVR_VID_UpdateViewport(wqcom_t *com)
{
	com->viewport.framenum = framenumber;
}

#endif
