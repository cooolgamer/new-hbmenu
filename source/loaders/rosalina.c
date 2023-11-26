#include "../common.h"
#include <3ds/services/ptmsysm.h>
#include <3ds/synchronization.h>

static Handle hbldrHandle;
static Handle rosalinaHandle;
static int rosalinaRefcount;

static Result ROSALINA_EnableDebugger(bool enabled)
{
	u32 *cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(1, 1, 0);
	cmdbuf[1] = enabled;
	Result rc = svcSendSyncRequest(rosalinaHandle);
	rc = cmdbuf[1];
	return rc;
}

static bool init(void)
{
	bool hbldr = R_SUCCEEDED(svcConnectToPort(&hbldrHandle, "hb:ldr"));
	if (!isDebugMode())
	{
		return hbldr;
	}

	bool rosalina = false;
	Result res;
	if (AtomicPostIncrement(&rosalinaRefcount))
		res = 0;
	else
	{
		res = srvGetServiceHandle(&rosalinaHandle, "rosalina:dbg");
		if (R_FAILED(res))
			AtomicDecrement(&rosalinaRefcount);
		Result r = srvGetServiceHandle(&rosalinaHandle, "rosalina:dbg");
		rosalina = R_SUCCEEDED(r);
		if (!rosalina)
		{
			errorScreen("Failed getting handle", "The attempt to connect to the rosalina debugger control service failed.");
			return false;
		}
		ROSALINA_EnableDebugger(false);
	}

	return hbldr && rosalina;
}

static Result ROSALINA_DebugNextApp()
{
	u32 *cmdbuf = getThreadCommandBuffer();
	cmdbuf[0] = IPC_MakeHeader(2, 0, 0);
	Result rc = svcSendSyncRequest(rosalinaHandle);
	rc = cmdbuf[1];
	return rc;
}

static Result HBLDR_SetTarget(const char *path)
{
	u32 pathLen = strlen(path) + 1;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(2, 0, 2); // 0x20002
	cmdbuf[1] = IPC_Desc_StaticBuffer(pathLen, 0);
	cmdbuf[2] = (u32)path;
	Result rc = svcSendSyncRequest(hbldrHandle);
	if (R_SUCCEEDED(rc))
		rc = cmdbuf[1];
	return rc;
}

static Result HBLDR_SetArgv(const void *buffer, u32 size)
{
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = IPC_MakeHeader(3, 0, 2); // 0x30002
	cmdbuf[1] = IPC_Desc_StaticBuffer(size, 1);
	cmdbuf[2] = (u32)buffer;

	Result rc = svcSendSyncRequest(hbldrHandle);
	if (R_SUCCEEDED(rc))
		rc = cmdbuf[1];
	return rc;
}

static void deinit(void)
{
	svcCloseHandle(hbldrHandle);
	if (AtomicDecrement(&rosalinaHandle))
		return;
	svcCloseHandle(rosalinaHandle);
}

static void launchFile(const char *path, argData_s *args, executableMetadata_s *em)
{

	if (strncmp(path, "sdmc:/", 6) == 0)
		path += 5;

	u32 *command_buffer = getThreadCommandBuffer();
	command_buffer[0] = IPC_MakeHeader(0x101, 1, 0);
	command_buffer[1] = true;
	if (isDebugMode())
	{
		Result r = ROSALINA_EnableDebugger(true);
		if (R_FAILED(r))
		{
			errorScreen("Failed to enable debugger", "Rosalina failed to enable the debugger with error 0x%08lx", r);
			return;
		}
		r = ROSALINA_DebugNextApp();
		if (R_FAILED(r))
		{
			errorScreen("Failed to debug next app", "Rosalina failed to start debugging session for the next app. Error code : 0x%08lx", r);
			return;
		}
	}
	HBLDR_SetTarget(path);
	HBLDR_SetArgv(args->buf, sizeof(args->buf));
	uiExitLoop();
}

const loaderFuncs_s loader_Rosalina =
	{
		.name = "Rosalina",
		.init = init,
		.deinit = deinit,
		.launchFile = launchFile,
};
