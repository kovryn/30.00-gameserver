#ifndef ABILITIES
#define ABILITIES
#pragma once
#include "Utils.h"
inline bool (*InternalTryActivateAbility)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle, FPredictionKey, UGameplayAbility**, void*, const FGameplayEventData*) = decltype(InternalTryActivateAbility)(InSDKUtils::GetImageBase() + 0x7AEFE28);
inline FGameplayAbilitySpecHandle* (*InternalGiveAbility)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle*, FGameplayAbilitySpec) = decltype(InternalGiveAbility)(InSDKUtils::GetImageBase() + 0x7AEE18C);
inline __int64 (*SpecConstructor)(FGameplayAbilitySpec*, UObject*, int, int, UObject*) = decltype(SpecConstructor)(InSDKUtils::GetImageBase() + 0x7B28E6C);
inline FGameplayAbilitySpecHandle(*GiveAbilityAndActivateOnce)(UAbilitySystemComponent* ASC, FGameplayAbilitySpecHandle*, FGameplayAbilitySpec, __int64) = decltype(GiveAbilityAndActivateOnce)(__int64(GetModuleHandleW(0)) + 0x7AEF168);

FGameplayAbilitySpec* FindAbilitySpecFromHandle(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle)
{
    for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->ActivatableAbilities.Items)
    {
        if (Spec.Handle.Handle == Handle.Handle)
        {
            return &Spec;
        }
    }
    return nullptr;
}

void InternalServerTryActiveAbilityHook(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle, bool InputPressed, const FPredictionKey& PredictionKey, const FGameplayEventData* TriggerEventData)
{

    FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilitySystemComponent, Handle);
    if (!Spec)
    {
        std::cout << "InternalServerTryActiveAbility. Invalid SpecHandle!" << "\n";
        AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
        return;
    }
    const UGameplayAbility* AbilityToActivate = Spec->ability;

    if (!AbilityToActivate)
    {
        std::cout << "InternalServerTryActiveAbility. No Ability!" << "\n";
        return;
    }
    UGameplayAbility* InstancedAbility = nullptr;
    Spec->InputPressed = true;

    if (!InternalTryActivateAbility(AbilitySystemComponent, Handle, PredictionKey, &InstancedAbility, nullptr, TriggerEventData))
    {
        AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
        Spec->InputPressed = false;
        AbilitySystemComponent->ActivatableAbilities.MarkItemDirty(*Spec);
    }


}
void GiveAbility(UAbilitySystemComponent* AbilitySystemComponent, UGameplayAbility* GameplayAbility)
{
    if (!GameplayAbility)
        return;
    FGameplayAbilitySpec Spec;
    SpecConstructor(&Spec, GameplayAbility, 1, -1, nullptr);
    InternalGiveAbility(AbilitySystemComponent, &Spec.Handle, Spec);
}

void GiveAbilitySet(AFortPlayerControllerAthena* PC, UFortAbilitySet* Set)
{
    if (Set)
    {
        auto PlayerState = (AFortPlayerStateAthena*)(PC->PlayerState);

        for (size_t i = 0; i < Set->GameplayAbilities.Num(); i++)
        {
            UGameplayAbility* Ability = (UGameplayAbility*)Set->GameplayAbilities[i].Get()->DefaultObject;

            if (Ability) { GiveAbility(PlayerState->AbilitySystemComponent, Ability); }
        }

        for (int i = 0; i < Set->GrantedGameplayEffects.Num(); ++i)
        {

            UClass* GameplayEffect = Set->GrantedGameplayEffects[i].GameplayEffect.Get();
            float Level = Set->GrantedGameplayEffects[i].Level;
            if (!GameplayEffect)
                continue;

            PlayerState->AbilitySystemComponent->BP_ApplyGameplayEffectToSelf(GameplayEffect, Level, FGameplayEffectContextHandle());
        }
    }
}
void GiveDefaultAbilitySet(AFortPlayerControllerAthena* PC)
{
    auto PlayerState = (AFortPlayerStateAthena*)(PC->PlayerState);

    static UFortAbilitySet* DefaultAbilitySet = StaticLoadObject<UFortAbilitySet>("/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_AthenaPlayer.GAS_AthenaPlayer");
    GiveAbilitySet(PC, DefaultAbilitySet);
    static UFortAbilitySet* TacticalSprintAbilitySet = StaticLoadObject<UFortAbilitySet>("/TacticalSprintGame/Gameplay/AS_TacticalSprint.AS_TacticalSprint");
    GiveAbilitySet(PC, TacticalSprintAbilitySet);
    static UGameplayAbility* SlidingAbility =(UGameplayAbility*)StaticLoadObject<UClass>("/Game/Abilities/Player/Sliding/GA_Athena_Player_Slide.GA_Athena_Player_Slide_C")->DefaultObject;
    GiveAbility(PlayerState->AbilitySystemComponent, SlidingAbility);
}


#endif // !
