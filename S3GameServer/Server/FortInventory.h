#pragma once
#include "Utils.h"

namespace FortInventory {
	bool CompareGuids(FGuid Guid1, FGuid Guid2) {
		if (Guid1.A == Guid2.A
			&& Guid1.B == Guid2.B
			&& Guid1.C == Guid2.C
			&& Guid1.D == Guid2.D) {
			return true;
		}
		else {
			return false;
		}
	}

	FFortItemEntry* FindItemEntryByGuid(AFortPlayerController* PC, FGuid Guid)
	{
		for (auto& Entry : ((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.ReplicatedEntries)
		{
			if (CompareGuids(Entry.ItemGuid, Guid))
			{
				return &Entry;
			}
		}
		return nullptr;
	}

	FFortItemEntry* FindItemEntryByDef(AFortPlayerController* PC, UFortItemDefinition* ItemDef)
	{
		if (!PC || !PC->WorldInventory.ObjectPointer || !ItemDef)
			return nullptr;
		for (int i = 0; i < ((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.ReplicatedEntries.Num(); ++i)
		{
			if (((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.ReplicatedEntries[i].ItemDefinition == ItemDef)
			{
				return &((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.ReplicatedEntries[i];
			}
		}
		return nullptr;
	}

	void Update(AFortPlayerController* PC, FFortItemEntry* ItemEntry = nullptr)
	{
		PC->HandleWorldInventoryLocalUpdate();
		((AFortInventory*)PC->WorldInventory.ObjectPointer)->HandleInventoryLocalUpdate();
		((AFortInventory*)PC->WorldInventory.ObjectPointer)->bRequiresLocalUpdate = true;
		((AFortInventory*)PC->WorldInventory.ObjectPointer)->ForceNetUpdate();
		if (ItemEntry == nullptr)
			((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.MarkArrayDirty();
		else
			((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.MarkItemDirty(*ItemEntry);
	}

	void GiveItem(AFortPlayerController* PC, UFortItemDefinition* Def, int Count, int LoadedAmmo, bool bShouldAddToExistingStack = false)
	{
		if (!Def) {
			printf("Failed to give item to player since the ItemDefinition is NULL!\n");
			return;
		}

		if (bShouldAddToExistingStack) {
			for (FFortItemEntry& ItemEntry : ((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.ReplicatedEntries) {
				if (Def == ItemEntry.ItemDefinition) {
					ItemEntry.Count += Count;

					((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.MarkItemDirty(ItemEntry);
					Update(PC);
					break;
				}
			}
			return;
		}

		UFortWorldItem* Item = (UFortWorldItem*)Def->CreateTemporaryItemInstanceBP(Count, 0);
		Item->SetOwningControllerForTemporaryItem(PC);
		Item->ItemEntry.LoadedAmmo = LoadedAmmo;

		if (Item && Item->ItemEntry.ItemDefinition && (Def->IsA(UFortAmmoItemDefinition::StaticClass()) || Def->IsA(UFortResourceItemDefinition::StaticClass()))) {
			FFortItemEntryStateValue Value{};
			Value.IntValue = true;
			Value.StateType = EFortItemEntryState::ShouldShowItemToast;
			Item->ItemEntry.StateValues.Add(Value);
		}

		((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
		((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.ItemInstances.Add(Item);
		((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.MarkItemDirty(Item->ItemEntry);
		((AFortInventory*)PC->WorldInventory.ObjectPointer)->HandleInventoryLocalUpdate();

		printf("Given Item: %s\n", Def->GetFullName().c_str());
	}

	FFortItemEntry* GiveStack(AFortPlayerControllerAthena* PC, UFortItemDefinition* Def, int Count = 1, bool GiveLoadedAmmo = false, int LoadedAmmo = 0, bool Toast = false)
	{
		UFortWorldItem* Item = (UFortWorldItem*)Def->CreateTemporaryItemInstanceBP(Count, 0);

		Item->SetOwningControllerForTemporaryItem(PC);
		Item->ItemEntry.ItemDefinition = Def;
		Item->ItemEntry.Count = Count;


		if (GiveLoadedAmmo)
		{
			Item->ItemEntry.LoadedAmmo = LoadedAmmo;
		}
		Item->ItemEntry.ReplicationKey++;

		((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
		((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.ItemInstances.Add(Item);

		Update(PC, nullptr);
		return &Item->ItemEntry;
	}

	void RemoveItem(AFortPlayerController* PC, UFortItemDefinition* Def, int Count = INT_MAX) {
		for (int i = 0; i < ((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.ReplicatedEntries.Num(); i++) {
			FFortItemEntry& ItemEntry = ((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.ReplicatedEntries[i];
			if (Def == ItemEntry.ItemDefinition) {
				ItemEntry.Count -= Count;
				if (ItemEntry.Count <= 0) {
					((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.ReplicatedEntries[i].StateValues.Free();
					((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.ReplicatedEntries.Remove(i);

					for (int i = 0; i < ((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.ItemInstances.Num(); i++)
					{
						UFortWorldItem* WorldItem = ((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.ItemInstances[i];
						if (WorldItem->ItemEntry.ItemDefinition == Def) {
							((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.ItemInstances.Remove(i);
						}
					}
				}
				else {
					Update(PC, &ItemEntry);
				}
				break;
			}
		}
		Update(PC);
	}

	UFortWorldItem* FindItemInstanceByDef(AFortInventory* inv, UFortItemDefinition* ItemDefinition)
	{
		auto& ItemInstances = inv->Inventory.ItemInstances;

		for (int i = 0; i < ItemInstances.Num(); i++)
		{
			auto ItemInstance = ItemInstances[i];

			if (ItemInstance->ItemEntry.ItemDefinition == ItemDefinition)
				return ItemInstance;
		}

		return nullptr;
	}

	UFortWorldItem* FindItemInstanceByGuid(AFortInventory* inv, const FGuid& Guid)
	{
		auto& ItemInstances = inv->Inventory.ItemInstances;

		for (int i = 0; i < ItemInstances.Num(); i++)
		{
			auto ItemInstance = ItemInstances[i];

			if (CompareGuids(ItemInstance->ItemEntry.ItemGuid, Guid))
				return ItemInstance;
		}

		return nullptr;
	}

	bool IsPrimaryQuickbar(UFortItemDefinition* Def)
	{
		return
			Def->IsA(UFortConsumableItemDefinition::StaticClass()) ||
			Def->IsA(UFortWeaponRangedItemDefinition::StaticClass()) ||
			Def->IsA(UFortGadgetItemDefinition::StaticClass());
	}

	EFortQuickBars GetQuickBars(UFortItemDefinition* ItemDefinition)
	{
		if (!ItemDefinition->IsA(UFortWeaponMeleeItemDefinition::StaticClass()) && !ItemDefinition->IsA(UFortEditToolItemDefinition::StaticClass()) &&
			!ItemDefinition->IsA(UFortBuildingItemDefinition::StaticClass()) && !ItemDefinition->IsA(UFortAmmoItemDefinition::StaticClass()) && !ItemDefinition->IsA(UFortResourceItemDefinition::StaticClass()) && !ItemDefinition->IsA(UFortTrapItemDefinition::StaticClass()))
			return EFortQuickBars::Primary;

		return EFortQuickBars::Secondary;
	}

	bool IsInventoryFull(AFortPlayerController* PC)
	{
		int ItemNumber = 0;
		auto InstancesPtr = &((AFortInventory*)PC->WorldInventory.ObjectPointer)->Inventory.ItemInstances;
		for (int i = 0; i < InstancesPtr->Num(); i++)
		{
			if (InstancesPtr->operator[](i))
			{
				if (GetQuickBars((UFortItemDefinition*)InstancesPtr->operator[](i)->ItemEntry.ItemDefinition) == EFortQuickBars::Primary)
				{
					if (InstancesPtr->operator[](i)->ItemEntry.ItemDefinition->Name.ToString().contains("VictoryCrown")) continue;
					ItemNumber++;

					if (ItemNumber >= 5)
					{
						break;
					}
				}
			}
		}

		return ItemNumber >= 5;
	}
}