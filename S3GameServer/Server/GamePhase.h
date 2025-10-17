#ifndef GAMEPHASE
#define GAMEPHASE
#pragma once
#include "Utils.h"
#define IMAGEBASE uint64_t(GetModuleHandle(0))


void SetGamePhase(UObject* WorldContextObject, EAthenaGamePhase GamePhase)
{
    auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(WorldContextObject);
    auto OldGamePhase = GamePhaseLogic->GamePhase;
    GamePhaseLogic->GamePhase = GamePhase;
    GamePhaseLogic->OnRep_GamePhase(OldGamePhase);
}

void SetGamePhaseStep(UObject* WorldContextObject, EAthenaGamePhaseStep GamePhaseStep)
{
    auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(WorldContextObject);
    GamePhaseLogic->GamePhaseStep = GamePhaseStep;
    GamePhaseLogic->HandleGamePhaseStepChanged(GamePhaseStep);
}


class UFunction* FindFunctionByFName(UClass* Class, FName FuncName)
{
	for (const UStruct* Clss = Class; Clss; Clss = Clss->Super)
	{
		for (UField* Field = Clss->Children; Field; Field = Field->Next)
		{
			if (Field->HasTypeFlag(EClassCastFlags::Function) && Field->Name == FuncName)
				return static_cast<class UFunction*>(Field);
		}
	}

	return nullptr;
}

void Tick()
{
	auto Time = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
	auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());

	static bool gettingReady = false;
	if (!gettingReady)
	{
		if (((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && GamePhaseLogic->WarmupCountdownEndTime - 10.f <= Time)
		{
			gettingReady = true;

			SetGamePhaseStep(UWorld::GetWorld(), EAthenaGamePhaseStep::GetReady);
		}
	}

	static bool startedBus = false;
	if (!startedBus)
	{
		if (((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && GamePhaseLogic->WarmupCountdownEndTime <= Time)
		{
			startedBus = true;

			auto Aircraft = GamePhaseLogic->Aircrafts_GameState[0].Get();
			auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
			auto& FlightInfo = GameState->MapInfo->FlightInfos[0];
			Aircraft->FlightElapsedTime = 0;
			Aircraft->DropStartTime = (float)Time + FlightInfo.TimeTillDropStart;
			Aircraft->DropEndTime = (float)Time + FlightInfo.TimeTillDropEnd;
			Aircraft->FlightStartTime = (float)Time;
			Aircraft->FlightEndTime = (float)Time + FlightInfo.TimeTillFlightEnd;
			Aircraft->ReplicatedFlightTimestamp = (float)Time;
			GamePhaseLogic->bAircraftIsLocked = true;
			for (auto& Player : ((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers)
			{
				auto Pawn = (AFortPlayerPawnAthena*)Player->Pawn;
				if (Pawn)
				{
					if (Pawn->Role == ENetRole::ROLE_Authority)
					{
						if (Pawn->bIsInAnyStorm)
						{
							Pawn->bIsInAnyStorm = false;
							Pawn->OnRep_IsInAnyStorm();
						}
					}
					Pawn->bIsInsideSafeZone = true;
					Pawn->OnRep_IsInsideSafeZone();
					for (auto& ScriptDelegate : Pawn->OnEnteredAircraft.InvocationList)
					{
						auto Object = ScriptDelegate.Object;
						Object->ProcessEvent(FindFunctionByFName(Object->Class, ScriptDelegate.FunctionName), nullptr);
					}
				}
				Player->ClientActivateSlot(EFortQuickBars::Primary, 0, 0.f, true, true);
				if (Pawn)
					Pawn->K2_DestroyActor();
				auto Reset = (void (*)(APlayerController*)) (IMAGEBASE + 0x6DD1E84);
				Reset(Player);
				/*Player->ClientGotoState(FName(L"Spectating"));*/
			}
			SetGamePhase(UWorld::GetWorld(), EAthenaGamePhase::Aircraft);
			SetGamePhaseStep(UWorld::GetWorld(), EAthenaGamePhaseStep::BusLocked);
		}
	}

	static bool busUnlocked = false;
	if (!busUnlocked)
	{
		if (((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && GamePhaseLogic->Aircrafts_GameState[0]->DropStartTime != -1 && GamePhaseLogic->Aircrafts_GameState[0]->DropStartTime <= Time)
		{
			busUnlocked = true;
			GamePhaseLogic->bAircraftIsLocked = false;
			SetGamePhaseStep(UWorld::GetWorld(), EAthenaGamePhaseStep::BusFlying);

		}
	}

	static bool startedZones = false;
	if (!startedZones)
	{
		if (((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && GamePhaseLogic->Aircrafts_GameState[0]->DropEndTime != -1 && GamePhaseLogic->Aircrafts_GameState[0]->DropEndTime <= Time)
		{
			startedZones = true;
			auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
			auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

			for (auto& Player : GameMode->AlivePlayers)
			{
				if (Player->IsInAircraft())
				{
					printf("test3.sys\n");
					Player->GetAircraftComponent()->ServerAttemptAircraftJump({});
				}
			}

			/*if (bLateGame)
			{
				GameState->GamePhase = EAthenaGamePhase::SafeZones;
				GameState->GamePhaseStep = EAthenaGamePhaseStep::StormHolding;
				GameState->OnRep_GamePhase(EAthenaGamePhase::Aircraft);
			}*/
			GamePhaseLogic->SafeZonesStartTime = (float)Time + 60.f;
			SetGamePhase(GameState, EAthenaGamePhase::SafeZones);
			SetGamePhaseStep(UWorld::GetWorld(), EAthenaGamePhaseStep::StormForming);
		}
	}

	static bool deletedBus = false;
	if (!deletedBus)
	{
		if (((AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode)->AlivePlayers.Num() > 0 && GamePhaseLogic->Aircrafts_GameState[0]->FlightEndTime != -1 && GamePhaseLogic->Aircrafts_GameState[0]->FlightEndTime <= Time)
		{
			deletedBus = true;
			auto Aircraft = GamePhaseLogic->Aircrafts_GameState[0].Get();
			Aircraft->K2_DestroyActor();
			GamePhaseLogic->Aircrafts_GameState.Clear();
			GamePhaseLogic->Aircrafts_GameMode.Clear();
		}
	}


}



#endif // !GAMEPHASE