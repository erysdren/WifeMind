#include "quakedef.h"
#include <arch/timer.h>
#include <kos/dbgio.h>
#include <kos/thread.h>

qboolean isDedicated = false;

double Sys_DoubleTime(void)
{
	uint64_t tick = timer_ms_gettime64();
	return (double)tick / 1000.0;
}

unsigned int Sys_Milliseconds(void)
{
	uint64_t tick = timer_ns_gettime64();
	return (unsigned int)(tick / 1000);
}

NORETURN void VARGS Sys_Error(const char *error, ...)
{
	va_list ap;
	char msg[1024];

	COM_WorkerAbort(error);

	va_start (ap, error);
	vsnprintf(msg, sizeof(msg), error, ap);
	va_end(ap);

	fputs(msg, stderr);
	fflush(stderr);

	exit(1);
}

void Sys_Sleep(double seconds)
{
	//yields to other processes/threads for a bit.
	thd_sleep(seconds * 1000.0);
}

void Sys_Quit(void)
{
#if 0
	Host_Shutdown ();
	//successful execution.
	//should go back to the system menu or something, or possibly longjmp out of the main loop.
	for (;;)
		;
	exit(1);
#endif
}

void Sys_Shutdown(void)
{

}

void Sys_Init(void)
{
	//register system-specific cvars here
}

/*prints to dedicated server console or debug output*/
void VARGS Sys_Printf(char *fmt, ...)
{
	va_list ap;
	char msg[1024];

	va_start (ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);

	fputs(msg, stderr);
	fflush(stderr);
}

/*returns the system video mode settings, ish*/
qboolean Sys_GetDesktopParameters(int *width, int *height, int *bpp, int *refreshrate)
{
	//FIXME: use XGetVideoStandard or XGetVideoFlags or something
	*width = 640;
	*height = 480;
	*bpp = 16;
	*refreshrate = 60;
	return true;
}

/*dedicated server type stuff*/
qboolean Sys_InitTerminal(void)
{
	return false; //failure
}

void Sys_CloseTerminal(void)
{

}

char *Sys_ConsoleInput(void)
{
	//returns any text typed on the stdin, when acting as a dedicated server.
	//this includes debugger commands if we're integrated with fteqccgui
	return NULL;
}

/*various system notifications*/
void Sys_ServerActivity(void)
{
	//player joined the server or said something. would normally flash the app in the system tray.
}

void Sys_RecentServer(char *command, char *target, char *title, char *desc)
{

}

/*check any system message queues here*/
void Sys_SendKeyEvents(void)
{

}

qboolean Sys_RandomBytes(qbyte *string, int len)
{
	//FIXME: should return some cryptographically strong random number data from some proper crypto library. C's rand function isn't really strong enough.
	return false;
}

/*filesystem stuff*/
void Sys_mkdir (const char *path)
{

}

qboolean Sys_rmdir (const char *path)
{
	return false;
}

qboolean Sys_remove (const char *path)
{
	return false;	//false for failure
}

qboolean Sys_Rename (const char *oldfname, const char *newfname)
{
	return false;	//false for failure
}

int Sys_EnumerateFiles (const char *gpath, const char *match, int (QDECL *func)(const char *fname, qofs_t fsize, time_t modtime, void *parm, searchpathfuncs_t *spath), void *parm, searchpathfuncs_t *spath)
{
	//if func returns false, abort with false return value, otherwise return true.
	//use wildcmd to compare two filenames
	//note that match may contain wildcards and multiple directories with multiple wildcards all over.
	return true;
}

/*consoles don't tend to need system clipboards, so this is fully internal to our engine*/
#define SYS_CLIPBOARD_SIZE  256
static char clipboard_buffer[SYS_CLIPBOARD_SIZE] = {0};
void Sys_Clipboard_PasteText(clipboardtype_t cbt, void (*callback)(void *cb, const char *utf8), void *ctx)
{
	callback(ctx, clipboard_buffer);
}
void Sys_SaveClipboard(clipboardtype_t cbt, const char *text)
{
 	Q_strncpyz(clipboard_buffer, text, SYS_CLIPBOARD_SIZE);
}

/*dynamic library stubs*/
dllhandle_t *Sys_LoadLibrary(const char *name, dllfunction_t *funcs)
{
	Con_Printf("Sys_LoadLibrary: %s\n", name);
	return NULL;
}

void Sys_CloseLibrary(dllhandle_t *lib)
{

}

void *Sys_GetAddressForName(dllhandle_t *module, const char *exportname)
{
	return NULL;	//unsupported
}

char *Sys_GetNameForAddress(dllhandle_t *module, void *address)
{
	return NULL;	//unsupported (on most platforms, actually)
}

qboolean Sys_GetFreeDiskSpace(const char *path, quint64_t *freespace)
{
	return false;
}

char *Sys_URIScheme_NeedsRegistering(void)
{
	return NULL;
}

int main( int argc, char **argv)
{
	float time, newtime, oldtime;
	quakeparms_t parms;

	memset(&parms, 0, sizeof(parms));

	parms.argc = argc;
	parms.argv = (const char**)argv;
#ifdef CONFIG_MANIFEST_TEXT
	parms.manifest = CONFIG_MANIFEST_TEXT;
#endif

	COM_InitArgv(parms.argc, parms.argv);
	TL_InitLanguages(parms.basedir);
	Host_Init(&parms);

	//main loop
	oldtime = 0;

	while (1)
	{
		double sleeptime;

		newtime = Sys_DoubleTime();
		time = newtime - oldtime;

		Sys_Printf("oldtime=%g newtime=%g time=%g sleeptime=%g\n", oldtime, newtime, time, sleeptime);

		sleeptime = Host_Frame(time);
		oldtime = newtime;

		if (sleeptime)
			Sys_Sleep(sleeptime);
	}

	return 0;
}

/*input stubs
in_generic.c should make these kinda useless, but sometimes you need more than the following three functions:
void IN_JoystickAxisEvent(unsigned int devid, int axis, float value);
void IN_KeyEvent(unsigned int devid, int down, int keycode, int unicode);
void IN_MouseMove(unsigned int devid, int abs, float x, float y, float z, float size);
that said, if they're enough, then just call those in Sys_SendKeyEvents. You can also call them from other threads just fine.
their devid values should be assigned only when a button is first pressed, or something, so controllers get assigned to seats in the order that they're pressed. so player 1 always hits a button first to ensure that they are player 1.
or just hardcode those based upon usbport numbers, but that's unreliable with usb hubs.
*/

enum controllertype_e INS_GetControllerType(int id)
{
	return CONTROLLER_NONE;
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

void INS_Shutdown (void)
{
}
void INS_ReInit (void)
{
}
void INS_Move(void)
{
	//accululates system-specific inputs on a per-seat basis.
}
void INS_Init (void)
{
}
void INS_Accumulate(void)	//input polling
{
}
void INS_Commands (void)	//used to Cbuf_AddText joystick button events in windows.
{
}
void INS_EnumerateDevices(void *ctx, void(*callback)(void *ctx, const char *type, const char *devicename, unsigned int *qdevid))
{
#if 0
	unsigned int i;
	for (i = 0; i < MAX_JOYSTICKS; i++)
		if (sdljoy[i].controller)
			callback(ctx, "joy", sdljoy[i].devname, &sdljoy[i].qdevid);
#endif
}

void INS_SetupControllerAudioDevices (qboolean enabled) // Not used
{
}

void INS_UpdateGrabs (void) // Not used
{
}

