#include "common.h"
#include "ui/menu.h"
#include "ui/error.h"
#include "ui/reboot.h"
#include "ui/titleselect.h"
#include "ui/netloader.h"
#include "ui/netsender.h"
#include "ui/background.h"

bool DEBUG_MODE = false;

const uiStateInfo_s g_uiStateTable[UI_STATE_MAX] =
	{
		[UI_STATE_MENU] = {
			.update = menuUpdate,
			.drawTop = menuDrawTop,
			.drawBot = menuDrawBot,
		},
		[UI_STATE_ERROR] = {
			.update = errorUpdate,
			.drawBot = errorDrawBot,
		},
		[UI_STATE_REBOOT] = {
			.update = rebootUpdate,
			.drawBot = rebootDrawBot,
		},
		[UI_STATE_TITLESELECT] = {
			.update = titleSelectUpdate,
			.drawBot = titleSelectDrawBot,
		},
		[UI_STATE_NETLOADER] = {
			.update = netloaderUpdate,
			.drawBot = netloaderDrawBot,
		},
		[UI_STATE_NETSENDER] = {
			.update = netsenderUpdate,
			.drawBot = netsenderDrawBot,
		},
		[UI_STATE_BACKGROUND] = {
			.update = backgroundUpdate,
			.drawTop = backgroundDrawTop,
			.drawBot = backgroundDrawBot,
		},
};

bool isDebugMode()
{
	return DEBUG_MODE;
}

static void startup(void *unused)
{
	menuLoadFileAssoc();
	menuScan("sdmc:/3ds");
	uiEnterState(UI_STATE_MENU);
}

const char *__romfs_path = "sdmc:/boot.3dsx";

int main()
{
	Result rc;
	FILE *file = fopen("sdmc:/debug_hbl.txt", "r");
	if (file)
	{
		DEBUG_MODE = true;
		fclose(file);
	}
	osSetSpeedupEnable(true);
	rc = romfsInit();
	if (R_FAILED(rc))
		svcBreak(USERBREAK_PANIC);

	menuStartupPath();
	hidSetRepeatParameters(20, 10);
	ptmuInit();
	uiInit();
	drawingInit();
	textInit();
	launchInit();
	workerInit();

	backgroundInit();

	workerSchedule(startup, NULL);

	// Main loop
	while (aptMainLoop())
	{
		if (!uiUpdate())
			break;
		drawingFrame();
	}

	netloaderExit();
	netsenderExit();
	launchExit();
	workerExit();
	textExit();
	drawingExit();
	romfsExit();
	ptmuExit();
	return 0;
}
