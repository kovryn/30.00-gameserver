#include <Windows.h>
#include <iostream>
#include <string>
#include "MinHook/MinHook.h"
#include "Server/Net.h"
#include <thread>
#include "Server/GameMode.h"
#include "Server/Misc.h"
#include "Server/PlayerController.h"
#include "Server/Pawn.h"

DWORD MainThread(HMODULE Module)
{
    AllocConsole();
    FILE* sFile;
    freopen_s(&sFile, "CONOUT$", "w", stdout);
    srand(time(0));

    MH_Initialize();

    while (!IsInLobby())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    Sleep(5000);

    uintptr_t BaseAddr = InSDKUtils::GetImageBase();
    SetConsoleTitleA("30.00 | Initializing");

    LPVOID ReadyToStartMatchAddr = (LPVOID)(BaseAddr + 0x8FD4388);
    LPVOID TickFlushAddr = (LPVOID)(BaseAddr + 0x1E66424);
    LPVOID WorldNetModeAddr = (LPVOID)(BaseAddr + 0x15AA5CC);
    LPVOID KickPlayerAddr = (LPVOID)(BaseAddr + 0x29702D8);
    LPVOID SpawnDefaultPawnAddr = (LPVOID)(BaseAddr + 0x8FD846C);
    LPVOID SetupInventoryAddr = (LPVOID)(BaseAddr + 0x8EAF67C);
    LPVOID ServerAcknowledgeAddr = (LPVOID)(BaseAddr + 0x9084A08);
    LPVOID ServerReadyToStartMatchAddr = (LPVOID)(BaseAddr + 0xA210874);
    LPVOID HandleMatchHasStartedAddr = (LPVOID)(BaseAddr + 0x8FCD25C); //1E65D44
    LPVOID GetMaxTickRateAddr = (LPVOID)(BaseAddr + 0x1E65D44);
    LPVOID DispatchRequestAddr = (LPVOID)(BaseAddr + 0x7C082F4);

    MH_CreateHook(DispatchRequestAddr, DispatchRequest, (LPVOID*)&DispatchRequestOG);
    MH_CreateHook(TickFlushAddr, TickFlushHook, (LPVOID*)&TickFlushOG);
    MH_CreateHook(GetMaxTickRateAddr, GetMaxTickRate, (LPVOID*)&GetMaxTickRateOG);
    MH_CreateHook(ReadyToStartMatchAddr, ReadyToStartMatchHook, (LPVOID*)&ReadyToStartMatchOG);
    MH_CreateHook(SpawnDefaultPawnAddr, SpawnDefaultPawnHook, nullptr);
    MH_CreateHook(WorldNetModeAddr, GetNetMode, nullptr);
    MH_CreateHook(KickPlayerAddr, ReturnTrue, nullptr);
    MH_CreateHook(SetupInventoryAddr, SetupInventory, (LPVOID*)&SetupInventoryOG);
    MH_CreateHook(ServerAcknowledgeAddr, ServerAcknowledgePossessionHook, nullptr);
    MH_CreateHook(ServerReadyToStartMatchAddr, ServerReadyToStartMatch, (LPVOID*)&ServerReadyToStartMatchOG);
    MH_CreateHook(HandleMatchHasStartedAddr, HandleMatchHasStarted, (LPVOID*)&HandleMatchHasStartedOG);
    VHook(UFortControllerComponent_Aircraft::GetDefaultObj(), 0xa7, ServerAttemptAircraftJump, nullptr);
  // VHook(AFortPlayerControllerAthena::GetDefaultObj(), 0x1fb, ServerPlayEmoteItem, nullptr);
  

    MH_EnableHook(MH_ALL_HOOKS);

    for (int i = 0; i < UObject::GObjects->Num(); i++)
    {
        auto Object = UObject::GObjects->GetByIndex(i);

        if (!Object)
            continue;

        if (Object->IsA(UAbilitySystemComponent::StaticClass()))
        {
            VHook(Object, 0x11B, InternalServerTryActiveAbilityHook, nullptr);
        }
    }

    *(bool*)(BaseAddr + 0x12A37C1A) = false; //GIsClient
    *(bool*)(BaseAddr + 0x12A37BAC) = true; //GIsServer
    UFortEngine::GetEngine()->GameInstance->LocalPlayers.Remove(0);
    UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(),  L"open Helios_Terrain", nullptr);
    UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), TEXT("log LogFortUIDirector off"), nullptr);
    return 0;
}


BOOL WINAPI DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, 0);
        break;
    case DLL_PROCESS_DETACH:
        FreeLibraryAndExitThread(hModule, 0);
        break;
    }
    return TRUE;
}
