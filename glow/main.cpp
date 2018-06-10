	#include "stdafx.h"
    #include <windows.h>
    #include <iostream>
	#include "Memory.h"
	#include "vector.h"
     
    typedef unsigned char uint8_t;
     
    template <typename T, size_t N>
     
    size_t countof(T(&array)[N])
    {
    	return N;
    }
     
    DWORD dwLocalPlayer;
    DWORD dwEntityList;
    DWORD dwGlow;
	DWORD dwJump;
	DWORD dwForceAttack;
	DWORD dwClientState_ViewAngles;
	DWORD dwClientState;
	DWORD dwClientState_state;
     
    DWORD dwTeam = 0xF0;
    DWORD dwDormant = 0xE9;
	DWORD dwFlags = 0x100;
	DWORD m_iShotsFired = 0xA2B0;
	DWORD m_aimPunchAngle = 0x301C;
	DWORD m_iCrosshairId = 0xB2A4;
	DWORD m_lifeState = 0x25B;
	DWORD DwEntitySize = 0x10;

    /* Glow Object structure in csgo */
    struct glow_t
    {
    	DWORD dwBase;
    	float r;
    	float g;
    	float b;
    	float a;
    	uint8_t unk1[16];
    	bool m_bRenderWhenOccluded;
    	bool m_bRenderWhenUnoccluded;
    	bool m_bFullBloom;
    	uint8_t unk2[14];
    };
     
    /* Entity structure in csgo */
    struct Entity
    {
    	DWORD dwBase;
    	int team;
    	bool is_dormant;
    };
     
    /* Player structure in csgo */
    struct Player
    {
    	DWORD dwBase;
    	bool isDormant;
    };
     
    process memory;
    process _modClient;
    process* mem;
    PModule modClient;
	PModule modEngine;
     
    int iFriendlies;
    int iEnemies;
     
    Entity entEnemies[32];
    Entity entFriendlies[32];
    Entity me;
     
    void update_entity_data(Entity* e, DWORD dwBase)
    {
    	int dormant = memory.Read<int>(dwBase + dwDormant);
    	e->dwBase = dwBase;
    	e->team = memory.Read<int>(dwBase + dwTeam);
    	e->is_dormant = dormant == 1;
    }
    /* Get Pointer To Client.dll*/
    PModule* GetClientModule() {
    	if (modClient.dwBase == 0 && modClient.dwSize == 0) {
    		modClient = memory.GetModule("client.dll");
    	}
    	return &modClient;
    }

	PModule* GetEngineModule() {
		if (modEngine.dwBase == 0 && modEngine.dwSize == 0) {
			modEngine = memory.GetModule("engine.dll");
		}
		return &modEngine;
	}
     
    Entity* GetEntityByBase(DWORD dwBase) {
     
    	for (int i = 0; i < iFriendlies; i++) {
    		if (dwBase == entFriendlies[i].dwBase) {
    			return &entFriendlies[i];
    		}
    	}
    	for (int i = 0; i < iEnemies; i++) {
    		if (dwBase == entEnemies[i].dwBase) {
    			return &entEnemies[i];
    		}
    	}
    	return nullptr;
    }
     
    /* offset updating class, that uses patterns to find memory addresses */
    class offset
    {
    private:
    	static void update_local_player() {
    		DWORD lpStart = mem->FindPatternArray(modClient.dwBase, modClient.dwSize, "xxx????xx????xxxxx?", 19, 0x8D, 0x34, 0x85, 0x0, 0x0, 0x0, 0x0, 0x89, 0x15, 0x0, 0x0, 0x0, 0x0, 0x8B, 0x41, 0x8, 0x8B, 0x48, 0x0);
    		DWORD lpP1 = mem->Read<DWORD>(lpStart + 3);
    		BYTE lpP2 = mem->Read<BYTE>(lpStart + 18);
    		dwLocalPlayer = (lpP1 + lpP2) - modClient.dwBase;
    	}
     
    	static void update_entity_list() {
    		DWORD elStart = mem->FindPatternArray(modClient.dwBase, modClient.dwSize, "x????xx?xxx", 11, 0x5, 0x0, 0x0, 0x0, 0x0, 0xC1, 0xE9, 0x0, 0x39, 0x48, 0x4);
    		DWORD elP1 = mem->Read<DWORD>(elStart + 1);
    		BYTE elP2 = mem->Read<BYTE>(elStart + 7);
    		dwEntityList = (elP1 + elP2) - modClient.dwBase;
    	}
     
    	static void update_glow() {
    		DWORD gpStart = mem->FindPatternArray(modClient.dwBase, modClient.dwSize, "xxx????xxxxx????????", 20, 0x0F, 0x11, 0x05, 0x0, 0x0, 0x0, 0x0, 0x83, 0xC8, 0x01, 0xC7, 0x05, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0);
    		dwGlow = mem->Read<DWORD>(gpStart + 3) - modClient.dwBase;
    	}

		static void update_Jump() {
			DWORD jStart = mem->FindPatternArray(modClient.dwBase, modClient.dwSize, "xx????xxxxxxx", 23, 0x8B, 0x0D, 0x0, 0x0, 0x0, 0x0, 0x8B, 0xD6, 0x8B, 0xC1, 0x83, 0xCA, 0x02);
			DWORD jOff = mem->Read<DWORD>(jStart + 2);
			dwJump = jOff - modClient.dwBase;
		}

		static void update_ViewAngles() {
			DWORD esStart = mem->FindPatternArray(modEngine.dwBase, modEngine.dwSize, "xxxx????xxxxx", 13, 0xF3, 0x0F, 0x11, 0x80, 0x0, 0x0, 0x0, 0x0, 0xD9, 0x46, 0x04, 0xD9, 0x05);
			dwClientState_ViewAngles = mem->Read<DWORD>(esStart + 4);
		}

		static void update_ClientState() {
			DWORD epStart = mem->FindPatternArray(modEngine.dwBase, modEngine.dwSize, "x????xxx?x?xxxx", 15, 0xA1, 0x0, 0x0, 0x0, 0x0, 0x33, 0xD2, 0x6A, 0x0, 0x6A, 0x0, 0x33, 0xC9, 0x89, 0xB0);
			dwClientState = mem->Read<DWORD>(epStart + 1) - modEngine.dwBase;
		}
		
		static void update_ClientState_state() {
		DWORD ewpStart = mem->FindPatternArray(modEngine.dwBase, modEngine.dwSize, "xx????xxxx", 11, 0x83, 0xB8, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0F, 0x94, 0xC0, 0xC3);
		dwClientState_state = mem->Read<DWORD>(ewpStart + 2);
		}

    public:
    	static void get_offset(process* m) {
    		mem = m;
    		modClient = mem->GetModule("client.dll");
			modEngine = mem->GetModule("engine.dll");
			update_local_player();
    		update_entity_list();
    		update_glow();
			update_Jump();
			update_ViewAngles();
			update_ClientState();
			update_ClientState_state();
    	}
     
    	//constantly scanning & updating our offsets
    	static DWORD WINAPI scan_offsets(LPVOID PARAM)
    	{
    		Entity players[64];
    		while (true) {
				Sleep(1);
    			DWORD playerBase = memory.Read<DWORD>(GetClientModule()->dwBase + dwLocalPlayer);
    			int cp = 0;
     
    			update_entity_data(&me, playerBase);
    			for (int i = 1; i < 64; i++) {
    				DWORD entBase = memory.Read<DWORD>((GetClientModule()->dwBase + dwEntityList) + i * 0x10);
     
    				if (entBase == NULL)
    					continue;
     
    				update_entity_data(&players[cp], entBase);
     
    				cp++;
    			}
     
    			int cf = 0, ce = 0;
     
    			for (int i = 0; i < cp; i++) {
    				if (players[i].team == me.team) {
    					entFriendlies[cf] = players[i];
    					cf++;
    				}
    				else {
    					entEnemies[ce] = players[i];
    					ce++;
    				}
    			}
    			iEnemies = ce;
    			iFriendlies = cf;
    		}
    	}
    };
     
     
    class virtualesp
    {
    private:
    	static void glow_player(DWORD mObj, float r, float g, float b)
    	{
    		memory.Write<float>(mObj + 0x4, r);
    		memory.Write<float>(mObj + 0x8, g);
    		memory.Write<float>(mObj + 0xC, b);
    		memory.Write<float>(mObj + 0x10, 1.0f);
    		memory.Write<BOOL>(mObj + 0x24, true);
    		memory.Write<BOOL>(mObj + 0x25, false);
    	}

    public:
    	static void start_engine() {
    		while (!memory.Attach("csgo.exe", PROCESS_ALL_ACCESS)) {
    			Sleep(100);
    		}
    		do {
    			Sleep(1000);
    			offset::get_offset(&memory);
    		} while (dwLocalPlayer < 65535);
    		CreateThread(NULL, NULL, &offset::scan_offsets, NULL, NULL, NULL);
    	}
     
    	static unsigned long __stdcall esp_thread(void*)
    	{
    		int objectCount;
    		DWORD pointerToGlow;
    		Entity* Player = NULL;
     
    		while (true)
    		{
				Sleep(10);
    			pointerToGlow = memory.Read<DWORD>(GetClientModule()->dwBase + dwGlow);
    			objectCount = memory.Read<DWORD>(GetClientModule()->dwBase + dwGlow + 0x4);
    			if (pointerToGlow != NULL && objectCount > 0)
    			{
    				for (int i = 0; i < objectCount; i++)
    				{
						//Sleep(1); //flicker

    					DWORD mObj = pointerToGlow + i * sizeof(glow_t);
    					glow_t glowObject = memory.Read<glow_t>(mObj);
    					Player = GetEntityByBase(glowObject.dwBase);
     
    					if (glowObject.dwBase == NULL || Player == nullptr || Player->is_dormant) {
    						continue;
    					}
    					if (me.team == Player->team) {
    						glow_player(mObj, 0, 255, 0); //change color here Red, Green, Blue
    					}
    					else {
    						glow_player(mObj, 255, 0, 0); //change color here Red, Green, Blue
    					}
    				}
    			}
    		}
    		return EXIT_SUCCESS;
    	}
    };

	DWORD GetEnginePointer()
	{
		return mem->Read<DWORD>(modEngine.dwBase + dwClientState);
	}

	bool IsInGame()
	{
		if (mem->Read<int>(GetEnginePointer() + dwClientState_state) == 6)
		{
			return true;
		}
		return false;
	}

	Vector GetViewAngles()
	{
		return mem->Read<Vector>(GetEnginePointer() + dwClientState_ViewAngles);
	}

	void SetViewAngles(Vector angles)
	{
		mem->Write<Vector>(GetEnginePointer() + dwClientState_ViewAngles, angles);
	}

	DWORD GetLocalPlayer()
	{
		return mem->Read<DWORD>(modClient.dwBase + dwLocalPlayer);
	}

	int GetShotsFired()
	{
		return mem->Read<int>(GetLocalPlayer() + m_iShotsFired);
	}

	int getLocalTeam()
	{
		return mem->Read<int>(GetLocalPlayer() + dwTeam);
	}

	int GetCrosshairId()
	{
		DWORD PlayerBase = GetLocalPlayer();
		if (PlayerBase)
		{
			return mem->Read<int>(PlayerBase + m_iCrosshairId) - 1;
		}
	}

	DWORD GetBaseEntity(int PlayerNumber)
	{
		return mem->Read<DWORD>(modClient.dwBase + dwEntityList + (DwEntitySize * PlayerNumber));
	}

	int GetTeam(int PlayerNumber)
	{
		DWORD BaseEntity = GetBaseEntity(PlayerNumber);
		if (BaseEntity)
		{
			return mem->Read<int>(BaseEntity + dwTeam);
		}
	}

	int GetTeam()
	{
		DWORD PlayerBase = GetLocalPlayer();
		if (PlayerBase)
		{
			return mem->Read<int>(PlayerBase + dwTeam);
		}
	}

	bool IsDead(int PlayerNumber)
	{
		DWORD BaseEntity = GetBaseEntity(PlayerNumber);
		if (BaseEntity)
		{
			return mem->Read<bool>(BaseEntity + m_lifeState);
		}
	}

	class cheats
	{
	public:
		static unsigned long __stdcall bhop_thread(void*)
		{
			while (true)
			{
				DWORD dwPla = memory.Read<DWORD>(GetClientModule()->dwBase + dwLocalPlayer);
				int flags = memory.Read<DWORD>(dwPla + dwFlags);

				if ((GetAsyncKeyState(VK_SPACE) & 0x8000) && (flags & 0x1 == 1)) {
					memory.Write<int>(GetClientModule()->dwBase + dwJump, 5);
					Sleep(25);
					memory.Write<int>(GetClientModule()->dwBase + dwJump, 4);
				}
				Sleep(1);
			}
			return EXIT_SUCCESS;
		}

		static unsigned long __stdcall norecoil_thread(void*)
		{
			Vector viewAngle;
			Vector punchAngle;
			Vector oldAngle;
			int rcsScale = 100;

			while (true)
			{
				if (IsInGame())
				{
					viewAngle = GetViewAngles();
					punchAngle = mem->Read<Vector>(GetLocalPlayer() + m_aimPunchAngle);

					if (GetShotsFired() > 1)
					{
						viewAngle.x -= (punchAngle.x - oldAngle.x) * (rcsScale * 0.02f);
						viewAngle.y -= (punchAngle.y - oldAngle.y) * (rcsScale * 0.02f);
						ClampVector(viewAngle);
						SetViewAngles(viewAngle);
						oldAngle.x = punchAngle.x;
						oldAngle.y = punchAngle.y;
					}
					else
					{
						oldAngle.x = punchAngle.x;
						oldAngle.y = punchAngle.y;
					}
				}
				Sleep(1);
			}
			return EXIT_SUCCESS;
		}

		static unsigned long __stdcall trigger_thread(void*)
		{
			while (true)
			{
				if (GetAsyncKeyState(VK_MENU) < 0 && VK_MENU != 1)
				{
					int PlayerNumber = GetCrosshairId();
					if (PlayerNumber < 64 && PlayerNumber >= 0 && GetTeam(PlayerNumber) != GetTeam() && IsDead(PlayerNumber) != true)
					{
						mouse_event(MOUSEEVENTF_LEFTDOWN, NULL, NULL, NULL, NULL);
						Sleep(200);
						mouse_event(MOUSEEVENTF_LEFTUP, NULL, NULL, NULL, NULL);
						Sleep(30);
					}
					else
					{
						Sleep(1);
					}
				}
				Sleep(1);
			}
			return EXIT_SUCCESS;
		}
	};
	 
    int main() 
    {
		SetConsoleTitle("Skeet Tapper");
    	bool glow = false;
		bool bhop = false;
		bool norecoil = false;
		bool trigger = false;

    	HANDLE ESP = NULL;
		HANDLE BHOP = NULL;
		HANDLE NORECOIL = NULL;
		HANDLE TRIGGER = NULL;

    	virtualesp::start_engine();

    	std::cout << "press F1 to toggle Glow!" << std::endl;
		std::cout << "press F2 to toggle Bhop!" << std::endl;
		std::cout << "press F3 to toggle Norecoil!" << std::endl;
		std::cout << "press F4 to toggle Triggerbot!" << std::endl;
		std::cout << "Triggerbot is binded to Alt!" << std::endl;
		

		std::cout << "" << std::endl;
		std::cout << "Offsets" << std::endl;
		std::cout << "" << std::endl;
		std::cout << "Localplayer = 0x" << std::hex << dwLocalPlayer << std::endl;
		std::cout << "Entitylist = 0x" << std::hex << dwEntityList << std::endl;
		std::cout << "Glowobject = 0x" << std::hex << dwGlow << std::endl;
		std::cout << "Forcejump = 0x" << std::hex << dwJump << std::endl;
		std::cout << "Viewangles = 0x" << std::hex << dwClientState_ViewAngles << std::endl;
		std::cout << "Clientstate = 0x" << std::hex << dwClientState << std::endl;
		std::cout << "Clientstate_state = 0x" << std::hex << dwClientState_state << std::endl;
		std::cout << "" << std::endl;

    	while (TRUE)
    	{
			Sleep(1);
    		if (GetAsyncKeyState(VK_F1) & 1) {
    			glow = !glow;
    			if (glow) {
    				std::cout << "Glow: on" << std::endl;
    				ESP = CreateThread(NULL, NULL, &virtualesp::esp_thread, NULL, NULL, NULL);
    			}
    			else {
    				std::cout << "Glow: off" << std::endl;
    				TerminateThread(ESP, 0);
    				CloseHandle(ESP);
    			}
    		}

			if (GetAsyncKeyState(VK_F2) & 1) {
				bhop = !bhop;
				if (bhop) {
					std::cout << "Bhop: on" << std::endl;
					BHOP = CreateThread(NULL, NULL, &cheats::bhop_thread, NULL, NULL, NULL);
				}
				else {
					std::cout << "Bhop: off" << std::endl;
					TerminateThread(BHOP, 0);
					CloseHandle(BHOP);
				}
			}

			if (GetAsyncKeyState(VK_F3) & 1) {
				norecoil = !norecoil;
				if (norecoil) {
					std::cout << "Norecoil: on" << std::endl;
					NORECOIL = CreateThread(NULL, NULL, &cheats::norecoil_thread, NULL, NULL, NULL);
				}
				else {
					std::cout << "Norecoil: off" << std::endl;
					TerminateThread(NORECOIL, 0);
					CloseHandle(NORECOIL);
				}
			}

			if (GetAsyncKeyState(VK_F4) & 1) {
				trigger = !trigger;
				if (trigger) {
					std::cout << "Trigger: on" << std::endl;
					TRIGGER = CreateThread(NULL, NULL, &cheats::trigger_thread, NULL, NULL, NULL);
				}
				else {
					std::cout << "Trigger: off" << std::endl;
					TerminateThread(TRIGGER, 0);
					CloseHandle(TRIGGER);
				}
			}
    	}
    }
