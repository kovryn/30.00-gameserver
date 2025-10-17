#ifndef PLAYERCONTROLLER
#define PLAYERCONTROLLER
#pragma once
#include "Utils.h"
#include "Abilities.h"

void (*SetupInventoryOG)(UFortControllerComponent_InventoryService* comp);
void SetupInventory(UFortControllerComponent_InventoryService* comp) {
	printf("SetupInventory Called!\n");

	SetupInventoryOG(comp);
	auto Owner = (AFortPlayerController*)comp->GetOwner();
	if (!Owner) {
		printf("Owner NULL!\n");
		return;
	}

	if (!Owner->WorldInventory.ObjectPointer) {
		auto NewInventory = SpawnActor<AFortInventory>({}, {}, Owner, Owner->WorldInventoryClass);
		Owner->WorldInventory.ObjectPointer = NewInventory;
		Owner->WorldInventory.InterfacePointer = GetInterfaceAddress(NewInventory, IFortInventoryInterface::StaticClass());
		Owner->bHasInitializedWorldInventory = true;

		printf("Setup Inventory!\n");
	}
}

void ServerAcknowledgePossessionHook(AFortPlayerControllerAthena* PlayerController, APawn* Pawn)
{
	((AFortPlayerStateAthena*)PlayerController->PlayerState)->HeroType = PlayerController->CosmeticLoadoutPC.Character->HeroDefinition;
	((void (*)(APlayerState*, APawn*)) (IMAGEBASE + 0x90D1040))(PlayerController->PlayerState, PlayerController->Pawn);
	PlayerController->AcknowledgedPawn = Pawn;
}

void (*ServerReadyToStartMatchOG)(AFortPlayerControllerAthena* PlayerController);
void ServerReadyToStartMatch(AFortPlayerControllerAthena* PlayerController) {
	ServerReadyToStartMatchOG(PlayerController);

	auto PlayerState = (AFortPlayerStateAthena*)(PlayerController->PlayerState);
	auto GameState = (AFortGameStateAthena*)(UWorld::GetWorld()->GameState);
	if (!PlayerState)
		return;

	GiveDefaultAbilitySet(PlayerController);
}

void ServerAttemptAircraftJump(UFortControllerComponent_Aircraft* This, FRotator& ClientRotation) {
	AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

	AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)This->GetOwner();
	AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;

	GameMode->RestartPlayer(PC);

	if (!PC || !PC->MyFortPawn) return;

	PC->MyFortPawn->BeginSkydiving(true);
	PC->ClientSetRotation(ClientRotation, false);
}

void ServerPlayEmoteItem(AFortPlayerControllerAthena* PC, UFortMontageItemDefinitionBase* EmoteAsset, float EmoteRandomNumber) {
	printf("ServerPlayEmoteItem Called!");

	if (!PC || !EmoteAsset)
		return;

	UClass* DanceAbility = StaticLoadObject<UClass>("/Game/Abilities/Emotes/GAB_Emote_Generic.GAB_Emote_Generic_C");
	UClass* SprayAbility = StaticLoadObject<UClass>("/Game/Abilities/Sprays/GAB_Spray_Generic.GAB_Spray_Generic_C");

	if (!DanceAbility || !SprayAbility)
		return;

	auto EmoteDef = (UAthenaDanceItemDefinition*)EmoteAsset;
	if (!EmoteDef)
		return;

	PC->MyFortPawn->bMovingEmote = EmoteDef->bMovingEmote;
	PC->MyFortPawn->EmoteWalkSpeed = EmoteDef->WalkForwardSpeed;
	PC->MyFortPawn->bMovingEmoteForwardOnly = EmoteDef->bMoveForwardOnly;
	PC->MyFortPawn->EmoteWalkSpeed = EmoteDef->WalkForwardSpeed;

	FGameplayAbilitySpec Spec{};
	UGameplayAbility* Ability = nullptr;

	if (EmoteAsset->IsA(UAthenaSprayItemDefinition::StaticClass()))
	{
		Ability = (UGameplayAbility*)SprayAbility->DefaultObject;
	}

	if (Ability == nullptr) {
		Ability = (UGameplayAbility*)DanceAbility->DefaultObject;
	}

	SpecConstructor(&Spec, Ability, 1, -1, EmoteDef); // 7B28E6C
	GiveAbilityAndActivateOnce(((AFortPlayerStateAthena*)PC->PlayerState)->AbilitySystemComponent, &Spec.Handle, Spec, 0); // 7AEF168
}


#endif
