#include <pthread.h>
#include <tier0/dbg.h>
#include <cstdio>
#include <GarrysMod/Lua/Interface.h>
#include <Platform.hpp>
#include <color.h>

lua_State* luaState = NULL;
volatile bool inspew = false; // volatile not needed but itll remind me

class SpewListener : public ILoggingListener
{
public:
	SpewListener(bool bQuietPrintf = false, bool bQuietDebugger = false) {}

	void Log(const LoggingContext_t* pContext, const char* pMsg) override
	{
		if (inspew || !luaState) return;

		const CLoggingSystem::LoggingChannel_t* chan = LoggingSystem_GetChannel(pContext->m_ChannelID);
		GarrysMod::Lua::ILuaBase* LUA = luaState->luabase;

		inspew = true;

		LUA->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
		LUA->GetField(-1, "hook");
		LUA->GetField(-1, "Run");

		LUA->PushString("EngineSpew"); // MAY TRIGGER LUA PANIC
		LUA->PushNumber((double)pContext->m_Flags);
		LUA->PushString(pMsg);
		LUA->PushString(chan->m_Name);
		LUA->PushNumber((double)pContext->m_Severity);

		// screw tables. Lets just push this in like this and be done with it.
		const Color* c = &pContext->m_Color;

		int Red, Green, Blue, Alpha = 0;
		c->GetColor(Red, Green, Blue, Alpha);

		LUA->PushNumber((float)Red);
		LUA->PushNumber((float)Green);
		LUA->PushNumber((float)Blue);

		if (LUA->PCall(8, 1, 0) != 0)
		{
			Warning("[EngineSpew error] %s\n", LUA->GetString());
		}

		LUA->Pop(3);
		inspew = false;
	}
};

ILoggingListener* listener = new SpewListener();
GMOD_MODULE_OPEN()
{
	luaState = LUA->GetState();
	LoggingSystem_PushLoggingState(false, false);
	LoggingSystem_RegisterLoggingListener(listener);

	return 0;
}

GMOD_MODULE_CLOSE()
{
	LoggingSystem_UnregisterLoggingListener(listener);
	LoggingSystem_PopLoggingState(false);
	luaState = NULL;
	
	return 0;
}