#ifndef NET
#define NET
#pragma once
#include "Utils.h"
#include "GamePhase.h"
#define UE_WITH_IRIS
bool bIsListening = false;

enum class EReplicationSystemSendPass : unsigned
{
	Invalid,
	PostTickDispatch,
	TickFlush,
};

struct FSendUpdateParams
{
	EReplicationSystemSendPass SendPass = EReplicationSystemSendPass::TickFlush;

	float DeltaSeconds = 0.f;
};

static void(*UpdateReplicationViews)(UNetDriver*) = decltype(UpdateReplicationViews)(InSDKUtils::GetImageBase() + 0x6CE5844);
static void(*PreSendUpdate)(UReplicationSystem*, FSendUpdateParams&) = decltype(PreSendUpdate)(InSDKUtils::GetImageBase() + 0x5F8FD5C);
void(__fastcall* SendClientAdjustmentOG)(void*) = decltype(SendClientAdjustmentOG)(InSDKUtils::GetImageBase() + 0x6A3F8A8);
void (*SetWorld)(UNetDriver*, UWorld*) = decltype(SetWorld)(InSDKUtils::GetImageBase() + 0x24BBD30);
UNetDriver* (*CreateNamedNetDriver_Local)(UEngine*, FWorldContext&, FName, FName) = decltype(CreateNamedNetDriver_Local)(InSDKUtils::GetImageBase() + 0x24BC7B0);
FWorldContext* (*GetWorldContextFromObject)(UEngine*, UWorld*) = decltype(GetWorldContextFromObject)(InSDKUtils::GetImageBase() + 0x1ABA190);//1ABA1C8
bool (*InitListen)(UNetDriver*, UWorld*, FURL&, bool, FString&) = decltype(InitListen)(InSDKUtils::GetImageBase() + 0x70453F0);

FWorldContext& GetWorldContext() {
	return *GetWorldContextFromObject(GetEngine(), GetWorld());
}

bool CreateDriverAndListen() {
	FName GameNetDriver = SDK::UKismetStringLibrary::Conv_StringToName(L"GameNetDriver");
	UNetDriver* NetDriver = CreateNamedNetDriver_Local(UEngine::GetEngine(), GetWorldContext(), GameNetDriver, GameNetDriver);
	if (!NetDriver)
		return false;

	NetDriver->NetDriverName = GameNetDriver;
	NetDriver->World = UWorld::GetWorld();

	FString Error;
	auto URL = FURL();
	URL.Port = 7777;

	if (!InitListen(NetDriver, UWorld::GetWorld(), URL, false, Error)) {
		return false;
	}

	SetWorld(NetDriver, UWorld::GetWorld());
	UWorld::GetWorld()->NetDriver = NetDriver;

	for (int i = 0; i < UWorld::GetWorld()->LevelCollections.Num(); i++)
	{
		UWorld::GetWorld()->LevelCollections[i].NetDriver = UWorld::GetWorld()->NetDriver;
	}

	bIsListening = true;
	return true;

}

void SendClientAdjustment(AFortPlayerController* PC)
{
	if (!PC)
		return;

	if (PC->AcknowledgedPawn != PC->Pawn && !PC->SpectatorPawn)
		return;

	auto Pawn = (ACharacter*)(PC->Pawn ? PC->Pawn : PC->SpectatorPawn);
	if (Pawn)
	{
		UCharacterMovementComponent* MoveComp = Pawn->CharacterMovement;
		if (MoveComp) {
			auto Interface = GetInterfaceAddress(Pawn->CharacterMovement, INetworkPredictionInterface::StaticClass());

			if (Interface)
			{
				SendClientAdjustmentOG(Interface);
			}
		}
	}
}

void SendClientMoveAdjustments(UNetDriver* Driver)
{
	for (UNetConnection* Connection : Driver->ClientConnections)
	{
		if (Connection == nullptr || Connection->ViewTarget == nullptr)
			continue;

		if (APlayerController* PC = Connection->PlayerController)
			SendClientAdjustment((AFortPlayerController*)PC);

		for (UNetConnection* ChildConnection : Connection->Children)
		{
			if (ChildConnection == nullptr)
				continue;

			if (APlayerController* PC = ChildConnection->PlayerController)
				SendClientAdjustment((AFortPlayerController*)PC);
		}
	}
}

int GetNetMode(__int64 a1) {
	return 1;
}

float (*GetMaxTickRateOG)(UEngine* Engine, float DeltaTime, bool bAllowFrameRateSmoothing);
float GetMaxTickRate(UEngine* Engine, float DeltaTime, bool bAllowFrameRateSmoothing) {
	
	return  30.f;
}

void* (*DispatchRequestOG)(void* Arg1, void* MCPData, int);
void* DispatchRequest(void* Arg1, void* MCPData, int)
{
	return DispatchRequestOG(Arg1, MCPData, 3);
}

void (*TickFlushOG)(UNetDriver*, float DeltaTime);
void TickFlushHook(UNetDriver* NetDriver, float DeltaTime) {
	if (NetDriver->ClientConnections.Num() > 0) {
		auto ReplicationSystem = *(UReplicationSystem**)(__int64(NetDriver) + 0x7C0);
		if (ReplicationSystem) {
			UpdateReplicationViews(NetDriver);
			SendClientMoveAdjustments(NetDriver);
			FSendUpdateParams Params = { .SendPass = EReplicationSystemSendPass::TickFlush, .DeltaSeconds = DeltaTime };
			PreSendUpdate(ReplicationSystem, Params);
		}
		Tick();
	}

	return TickFlushOG(NetDriver, DeltaTime);
}

#endif