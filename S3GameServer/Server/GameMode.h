#ifndef GAMEMODE
#define GAMEMODE
#pragma once
#include "Utils.h"
#include "Net.h"
#include "FortInventory.h"
#include "GamePhase.h"

bool(__fastcall* ReadyToStartMatchOG)(AFortGameModeBR*);
bool ReadyToStartMatchHook(AFortGameModeBR* GameMode)
{
    if (!GameMode || !GameMode->GameState)
        return false;

    auto result = ReadyToStartMatchOG(GameMode);

    auto GameState = (AFortGameStateBR*)(GameMode->GameState);

    if (!GameState->MapInfo)
        return false;

    static bool InitPlaylist = false;

    if (!InitPlaylist)
    {
        InitPlaylist = true;

        auto Playlist = UObject::FindObject<UFortPlaylistAthena>("FortPlaylistAthena Playlist_DefaultSolo.Playlist_DefaultSolo");

        GameMode->WarmupRequiredPlayerCount = 1;
        GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;

        GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
        GameState->CurrentPlaylistInfo.MarkArrayDirty();
        GameState->CurrentPlaylistId = Playlist->PlaylistId;
        GameMode->CurrentPlaylistId = Playlist->PlaylistId;
        GameMode->CurrentPlaylistName = Playlist->PlaylistName;

        GameMode->GameSession->MaxPlayers = 100;
        GameMode->PlaylistHotfixOriginalGCFrequency = Playlist->GarbageCollectionFrequency;

        GetGamePhaseLogic()->AirCraftBehavior = Playlist->AirCraftBehavior;
        GameState->DefaultParachuteDeployTraceForGroundDistance = 10000.0f;
    }

    if (GameState->Teams.Num() == 0) return false;

    TArray<AActor*> PlayerStarts;
    UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), AFortPlayerStartWarmup::StaticClass(), &PlayerStarts);

    int Spots = PlayerStarts.Num();
    auto InitializeFlightPath = (void(*)(AFortAthenaMapInfo*, AFortGameStateAthena*, UFortGameStateComponent_BattleRoyaleGamePhaseLogic*, bool, double, float, float)) (IMAGEBASE + 0x8F454DC);
    InitializeFlightPath(GameState->MapInfo, GameState, UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(GameState), false, 0.f, 0.f, 360.f);
    PlayerStarts.Clear();
    if (Spots == 0)
        return false;

    if (!bIsListening) {
        auto GamePhaseLogic = GetGamePhaseLogic();

        GameState->OnRep_CurrentPlaylistId();
        GameState->OnRep_CurrentPlaylistInfo();

        if (CreateDriverAndListen()) {
            GameMode->bWorldIsReady = true;
            SetConsoleTitleA("30.00 Gameserver | Listening");
        }
    }

    if (GameState->PlayersLeft > 0)
    {
        return true;
    }
  
    return false;
}

APawn* SpawnDefaultPawnHook(AFortGameModeBR* GameMode, AFortPlayerControllerAthena* NewPlayer, AActor* Spot) {

    auto Pawn = (AFortPlayerPawnAthena*)GameMode->SpawnDefaultPawnAtTransform(NewPlayer, Spot->GetTransform());
    printf("SpawnDefaultPawnFor: Spawned new pawn!\n");

    UFortWeaponMeleeItemDefinition* Pickaxe = StaticLoadObject<UFortWeaponMeleeItemDefinition>("/Game/Athena/Items/Weapons/WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01");
    FortInventory::GiveItem(NewPlayer, Pickaxe, 1, 1);
   
    return Pawn;
}

void SpawnAircrafts(AFortGameModeAthena* GameMode)
{
    auto GameState = (AFortGameStateAthena*)GameMode->GameState;
    auto Playlist = GameState->CurrentPlaylistInfo.BasePlaylist;
    auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(GameMode);

    int NumAircraftsToSpawn = 1;
    if (Playlist->AirCraftBehavior == EAirCraftBehavior::OpposingAirCraftForEachTeam)
        NumAircraftsToSpawn = GameState->TeamCount;

    TArray<AFortAthenaAircraft*> Aircrafts;
    for (int i = 0; i < NumAircraftsToSpawn; i++)
        Aircrafts.Add(AFortAthenaAircraft::SpawnAircraft(UWorld::GetWorld(), GameState->MapInfo->AircraftClass, GameState->MapInfo->FlightInfos[i]));

    GamePhaseLogic->SetAircrafts(Aircrafts);
    GamePhaseLogic->OnRep_Aircrafts();
}

void (*HandleMatchHasStartedOG)(AFortGameModeAthena* GameMode);
void HandleMatchHasStarted(AFortGameModeAthena* GameMode)
{
    printf("[HandleMatchHasStarted] Called!\n");
    HandleMatchHasStartedOG(GameMode);

    auto World = UWorld::GetWorld();
    if (!World)
    {
        printf("[HandleMatchHasStarted] World is nullptr!\n");
        return;
    }
    printf("[HandleMatchHasStarted] World OK: %p\n", World);

    auto Time = (float)UGameplayStatics::GetTimeSeconds(World);
    printf("[HandleMatchHasStarted] Time: %f\n", Time);

    auto WarmupDuration = 70.f;

    auto GamePhaseLogic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(GameMode);
    if (!GamePhaseLogic)
    {
        printf("[HandleMatchHasStarted] GamePhaseLogic is nullptr!\n");
        return;
    }
    printf("[HandleMatchHasStarted] GamePhaseLogic OK: %p\n", GamePhaseLogic);

    GamePhaseLogic->WarmupCountdownStartTime = Time;
    GamePhaseLogic->WarmupCountdownEndTime = Time + WarmupDuration;
    GamePhaseLogic->WarmupCountdownDuration = WarmupDuration;
    GamePhaseLogic->WarmupEarlyCountdownDuration = 0.f;

    auto GameState = (AFortGameStateAthena*)GameMode->GameState;
    if (!GameState)
    {
        printf("[HandleMatchHasStarted] GameState is nullptr!\n");
        return;
    }
    printf("[HandleMatchHasStarted] GameState OK: %p\n", GameState);

    if (!GameState->MapInfo)
    {
        printf("[HandleMatchHasStarted] GameState->MapInfo is nullptr!\n");
        return;
    }
    printf("[HandleMatchHasStarted] MapInfo OK: %p\n", GameState->MapInfo);

    if (GameState->MapInfo->FlightInfos.Num() <= 0)
    {
        printf("[HandleMatchHasStarted] FlightInfos empty!\n");
        return;
    }
    printf("[HandleMatchHasStarted] FlightInfos count: %d\n", GameState->MapInfo->FlightInfos.Num());

    auto AircraftClass = GameState->MapInfo->AircraftClass;
    if (!AircraftClass)
    {
        printf("[HandleMatchHasStarted] AircraftClass is nullptr!\n");
        return;
    }
    printf("[HandleMatchHasStarted] AircraftClass OK: %p\n", AircraftClass);

    TArray<AFortAthenaAircraft*> Aircrafts;
    auto SpawnedAircraft = AFortAthenaAircraft::SpawnAircraft(World, AircraftClass, GameState->MapInfo->FlightInfos[0]);
    if (!SpawnedAircraft)
    {
        printf("[HandleMatchHasStarted] SpawnAircraft returned nullptr!\n");
        return;
    }
    printf("[HandleMatchHasStarted] Spawned Aircraft: %p\n", SpawnedAircraft);

    Aircrafts.Add(SpawnedAircraft);

    GamePhaseLogic->SetAircrafts(Aircrafts);
    printf("[HandleMatchHasStarted] SetAircrafts called!\n");

    GamePhaseLogic->OnRep_Aircrafts();
    printf("[HandleMatchHasStarted] OnRep_Aircrafts called!\n");

    SetGamePhase(GameMode, EAthenaGamePhase::Warmup);
    printf("[HandleMatchHasStarted] SetGamePhase -> Warmup\n");

    SetGamePhaseStep(GameMode, EAthenaGamePhaseStep::Warmup);
    printf("[HandleMatchHasStarted] SetGamePhaseStep -> Warmup\n");
}


#endif // !GAMEMODE