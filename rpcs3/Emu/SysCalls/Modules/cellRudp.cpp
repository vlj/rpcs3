#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"

#include "sys_net.h"
#include "cellRudp.h"

extern Module cellRudp;

struct cellRudpInternal
{
	bool m_bInitialized;
	CellRudpAllocator allocator;
	vm::ptr<CellRudpEventHandler> handler;
	u32 argument;

	cellRudpInternal()
		: m_bInitialized(false)
	{
	}
};

cellRudpInternal cellRudpInstance;

s32 cellRudpInit(vm::ptr<CellRudpAllocator> allocator)
{
	cellRudp.Warning("cellRudpInit(allocator_addr=0x%x)", allocator.addr());

	if (cellRudpInstance.m_bInitialized)
	{
		cellRudp.Error("cellRudpInit(): cellRudp has already been initialized.");
		return CELL_RUDP_ERROR_ALREADY_INITIALIZED;
	}

	if (allocator)
	{
		cellRudpInstance.allocator = *allocator.get_ptr();
	}

	cellRudpInstance.m_bInitialized = true;

	return CELL_OK;
}

s32 cellRudpEnd()
{
	cellRudp.Log("cellRudpEnd()");

	if (!cellRudpInstance.m_bInitialized)
	{
		cellRudp.Error("cellRudpEnd(): cellRudp has not been initialized.");
		return CELL_RUDP_ERROR_NOT_INITIALIZED;
	}

	cellRudpInstance.m_bInitialized = false;

	return CELL_OK;
}

s32 cellRudpEnableInternalIOThread()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpSetEventHandler(vm::ptr<CellRudpEventHandler> handler, vm::ptr<u32> arg)
{
	cellRudp.Todo("cellRudpSetEventHandler(handler=0x%x, arg_addr=0x%x)", handler, arg.addr());

	if (!cellRudpInstance.m_bInitialized)
	{
		cellRudp.Error("cellRudpInit(): cellRudp has not been initialized.");
		return CELL_RUDP_ERROR_NOT_INITIALIZED;
	}

	if (arg)
	{
		cellRudpInstance.argument = *arg.get_ptr();
	}

	cellRudpInstance.handler = handler;

	return CELL_OK;
}

s32 cellRudpSetMaxSegmentSize(u16 mss)
{
	cellRudp.Todo("cellRudpSetMaxSegmentSize(mss=%d)", mss);
	return CELL_OK;
}

s32 cellRudpGetMaxSegmentSize()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpCreateContext()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpSetOption()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpGetOption()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpGetContextStatus()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpGetStatus()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpGetLocalInfo()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpGetRemoteInfo()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpBind()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpInitiate()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpActivate()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpTerminate()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpRead()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpWrite()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpGetSizeReadable()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpGetSizeWritable()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpFlush()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpPollCreate()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpPollDestroy()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpPollControl()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpPollWait()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpNetReceived()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

s32 cellRudpProcessEvents()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

Module cellRudp("cellRudp", []()
{
	cellRudpInstance.m_bInitialized = false;

	REG_FUNC(cellRudp, cellRudpInit);
	REG_FUNC(cellRudp, cellRudpEnd);
	REG_FUNC(cellRudp, cellRudpEnableInternalIOThread);
	REG_FUNC(cellRudp, cellRudpSetEventHandler);
	REG_FUNC(cellRudp, cellRudpSetMaxSegmentSize);
	REG_FUNC(cellRudp, cellRudpGetMaxSegmentSize);

	REG_FUNC(cellRudp, cellRudpCreateContext);
	REG_FUNC(cellRudp, cellRudpSetOption);
	REG_FUNC(cellRudp, cellRudpGetOption);

	REG_FUNC(cellRudp, cellRudpGetContextStatus);
	REG_FUNC(cellRudp, cellRudpGetStatus);
	REG_FUNC(cellRudp, cellRudpGetLocalInfo);
	REG_FUNC(cellRudp, cellRudpGetRemoteInfo);

	REG_FUNC(cellRudp, cellRudpBind);
	REG_FUNC(cellRudp, cellRudpInitiate);
	REG_FUNC(cellRudp, cellRudpActivate);
	REG_FUNC(cellRudp, cellRudpTerminate);

	REG_FUNC(cellRudp, cellRudpRead);
	REG_FUNC(cellRudp, cellRudpWrite);
	REG_FUNC(cellRudp, cellRudpGetSizeReadable);
	REG_FUNC(cellRudp, cellRudpGetSizeWritable);
	REG_FUNC(cellRudp, cellRudpFlush);

	REG_FUNC(cellRudp, cellRudpPollCreate);
	REG_FUNC(cellRudp, cellRudpPollDestroy);
	REG_FUNC(cellRudp, cellRudpPollControl);
	REG_FUNC(cellRudp, cellRudpPollWait);
	//REG_FUNC(cellRudp, cellRudpPollCancel);

	REG_FUNC(cellRudp, cellRudpNetReceived);
	REG_FUNC(cellRudp, cellRudpProcessEvents);
});
