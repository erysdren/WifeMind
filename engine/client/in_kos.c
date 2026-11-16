#include "quakedef.h"
#include <arch/timer.h>

static int joy_count;

enum controllertype_e INS_GetControllerType(int id)
{
	int j;
	for (j = 0; j < joy_count; j++)
	{
		if (wjoy[j].devid == id)
		{
			if (wjoy[j].devstate == DS_NOTPRESENT)
				return CONTROLLER_NONE;
			if (wjoy[j].isxinput)
				return CONTROLLER_XBOX;	//we don't really know, but we're using it through an xbox-specific api, so...
			return CONTROLLER_UNKNOWN;	//the legacy joy API.
		}
	}
	return CONTROLLER_NONE;	//no idea what you're talking about.
}

void INS_Rumble(int id, quint16_t amp_low, quint16_t amp_high, quint32_t duration)
{
	//Con_DPrintf(CON_WARNING "Rumble is unavailable on this platform\n");
}

void INS_RumbleTriggers(int id, quint16_t left, quint16_t right, quint32_t duration)
{
	//Con_DPrintf(CON_WARNING "Trigger rumble is unavailable on this platform\n");
}

void INS_SetLEDColor(int id, vec3_t color)
{
	//Con_DPrintf(CON_WARNING "Game-Pad LED colors are unavailable on this platform\n");
}

void INS_SetTriggerFX(int id, const void *data, size_t size)
{
	//Con_DPrintf(CON_WARNING "Trigger FX are unavailable on this platform\n");
}
