// Fill out your copyright notice in the Description page of Project Settings.


#include "Inventory/InventoryComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Framework/CAssetManager.h"
#include "Framework/CGameState.h"
#include "GameplayEffect.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/CGameplayEffectSettings.h"
#include "GAS/CHeroAttributeSet.h"
#include "Inventory/PA_ShopItem.h"
#include "Net/UnrealNetwork.h"
#include "Player/CPlayerState.h"

void FInventoryEntry::PreReplicatedRemove(const FInventoryList& InArray)
{
	if ( InArray.OwnerComponent )
	{
		InArray.OwnerComponent->HandleReplicatedEntryRemoved(*this);
	}
}

void FInventoryEntry::PostReplicatedAdd(const FInventoryList& InArray)
{
	if ( InArray.OwnerComponent )
	{
		InArray.OwnerComponent->HandleReplicatedEntryAdded(*this);
	}
}

void FInventoryEntry::PostReplicatedChange(const FInventoryList& InArray)
{
	if ( InArray.OwnerComponent )
	{
		InArray.OwnerComponent->HandleReplicatedEntryChanged(*this);
	}
}

void FInventoryList::AddEntry(const UInventoryItem& Item)
{
	FInventoryEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Handle = Item.GetHandle();
	NewEntry.ShopItem = Item.GetShopItem();
	NewEntry.StackCount = Item.GetStackCount();
	NewEntry.GrantedAbilitySpecHandle = Item.GetGrantedAbilitySpecHandle();

	MarkItemDirty(NewEntry);
}

void FInventoryList::RemoveEntry(const FInventoryItemHandle& Handle)
{
	const int32 NumRemoved = Entries.RemoveAll([ &Handle ] (const FInventoryEntry& Entry)
		{
			return Entry.Handle == Handle;
		});

	if ( NumRemoved > 0 )
	{
		MarkArrayDirty();
	}
}

void FInventoryList::UpdateStackCount(const FInventoryItemHandle& Handle, int32 NewStackCount)
{
	for ( FInventoryEntry& Entry : Entries )
	{
		if ( Entry.Handle == Handle )
		{
			Entry.StackCount = NewStackCount;
			MarkItemDirty(Entry);
			return;
		}
	}
}

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::PostInitProperties()
{
	Super::PostInitProperties();

	InventoryList.OwnerComponent = this;
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UInventoryComponent, InventoryList, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UInventoryComponent, UndoCount, COND_OwnerOnly);
}

void UInventoryComponent::TryUndo()
{
	if ( !CanUndo() )
		return;

	Server_UndoLastTransaction();
}

void UInventoryComponent::TryActivateItem(const FInventoryItemHandle& ItemHandle)
{
	UInventoryItem* InventoryItem = GetInventoryItemByHandle(ItemHandle);

	if (!InventoryItem)
		return;

	Server_ActivateItem(ItemHandle);
}

void UInventoryComponent::TryPurchase(const UPA_ShopItem* ItemToPurchase)
{
	if ( !OwnerAbilitySystemComponent )
		return;

	EPurchaseResult Result = EvaluatePurchase(ItemToPurchase);

	if ( Result != EPurchaseResult::Success )
	{
		const bool bHasFallback = Result == EPurchaseResult::NotEnoughGold && FindFallbackIngredientPurchase(ItemToPurchase) != nullptr;
		if ( !bHasFallback )
		{
			OnPurchaseFailed.Broadcast(Result, ItemToPurchase);
			return;
		}
	}

	Server_Purchase(ItemToPurchase);
}

void UInventoryComponent::SellItem(const FInventoryItemHandle& ItemHandle)
{
	if ( !GetInventoryItemByHandle(ItemHandle) )
		return;

	Server_SellItem(ItemHandle);
}

bool UInventoryComponent::CanPurchase(const UPA_ShopItem* Item) const
{
	if ( !Item )
		return false;

	if (EvaluatePurchase(Item) == EPurchaseResult::Success)
		return true;

	return false;
}

float UInventoryComponent::GetGold() const
{
	bool bFound = false;

	if (OwnerAbilitySystemComponent)
	{
		float Gold = OwnerAbilitySystemComponent->GetGameplayAttributeValue(UCHeroAttributeSet::GetGoldAttribute(), bFound);

		if(bFound)
			return Gold;
	}

	return 0.f;
}

void UInventoryComponent::ItemSlotChanged(const FInventoryItemHandle& Handle, int NewSlotNumber)
{
	if (UInventoryItem* FoundItem = GetInventoryItemByHandle(Handle))
	{
		FoundItem->SetSlot(NewSlotNumber);
	}
}

UInventoryItem* UInventoryComponent::GetInventoryItemByHandle(const FInventoryItemHandle& Handle) const
{
	UInventoryItem* const* FoundItem = InventoryMap.Find(Handle);
	if (FoundItem)
	{
		return *FoundItem;
	}

	return nullptr;
}

bool UInventoryComponent::IsFullFor(const UPA_ShopItem* Item) const
{
	if(!Item)
		return false;

	if (IsAllSlotOccupied())
	{
		return GetAvailableStackForItem(Item) == nullptr;
	}

	return false;
}

bool UInventoryComponent::IsAllSlotOccupied() const
{
	return InventoryMap.Num() >= GetCapacity();
}

UInventoryItem* UInventoryComponent::GetAvailableStackForItem(const UPA_ShopItem* Item) const
{
	if (!Item->GetIsStackable())
		return nullptr;

	for (const TPair<FInventoryItemHandle, UInventoryItem*> ItemPair : InventoryMap)
	{
		if (ItemPair.Value && ItemPair.Value->IsForItem(Item) && !ItemPair.Value->IsStackFull())
		{
			return ItemPair.Value;
		}
	}

	return nullptr;
}

bool UInventoryComponent::FindIngredientForItem(const UPA_ShopItem* Item, TArray<UInventoryItem*>& OutIngredients, const TArray<const UPA_ShopItem*>& IngredientToIgnore)
{
	const FItemCollection* Ingredients = UCAssetManager::Get().GetIngredientForItem(Item);
	if (!Ingredients)
		return false;

	bool bAllFound = true;
	for (const UPA_ShopItem* Ingredient : Ingredients->GetItems())
	{
		if (IngredientToIgnore.Contains(Ingredient))
			continue;

		UInventoryItem* FoundItem = TryGetItemForShopItem(Ingredient);
		if (!FoundItem)
		{
			bAllFound = false;
			break;
		}

		OutIngredients.Add(FoundItem);
	}

	return bAllFound;
}

UInventoryItem* UInventoryComponent::TryGetItemForShopItem(const UPA_ShopItem* Item) const
{
	if (!Item)
		return nullptr;
	
	for (const TPair<FInventoryItemHandle, UInventoryItem*>& ItemHandlePair : InventoryMap)
	{
		if (ItemHandlePair.Value && ItemHandlePair.Value->GetShopItem() == Item)
		{
			return ItemHandlePair.Value;
		}
	}

	return nullptr;
}

void UInventoryComponent::TryActivateItemInSlot(int SlotNumber)
{
	for (TPair<FInventoryItemHandle, UInventoryItem*>& ItemPair : InventoryMap)
	{
		if (ItemPair.Value->GetItemSlot() == SlotNumber)
		{
			Server_ActivateItem(ItemPair.Key);
			return;
		}
	}
}

TArray<UInventoryItem*> UInventoryComponent::GetAllItems() const
{
	TArray<UInventoryItem*> Items;
	InventoryMap.GenerateValueArray(Items);

	return Items;
}

void UInventoryComponent::SyncScoreboardItems()
{
	if ( !GetOwner()->HasAuthority() )
		return;

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	ACPlayerState* PS = OwnerPawn ? OwnerPawn->GetPlayerState<ACPlayerState>() : nullptr;
	if ( !PS )
		return;

	TArray<FScoreboardItemEntry> Snapshot;
	for ( const TPair<FInventoryItemHandle, UInventoryItem*>& Pair : InventoryMap )
	{
		if ( Pair.Value && Pair.Value->GetShopItem() )
		{
			FScoreboardItemEntry Entry;
			Entry.ShopItem = Pair.Value->GetShopItem();
			Entry.StackCount = Pair.Value->GetStackCount();
			Snapshot.Add(Entry);
		}
	}

	PS->UpdateScoreboardItems(Snapshot);
}

bool UInventoryComponent::TryCombinePurchase(const UPA_ShopItem* ItemToCombine, FInventoryTransaction* OutTransaction)
{
	if ( !GetOwner()->HasAuthority() || !ItemToCombine )
		return false;

	TArray<UInventoryItem*> OwnedIngredients;
	const float CombinePrice = GetEffectivePrice(ItemToCombine, &OwnedIngredients);

	if ( OwnedIngredients.Num() == 0 )
		return false;

	if ( GetGold() < CombinePrice )
		return false;

	OwnerAbilitySystemComponent->ApplyModToAttribute(UCHeroAttributeSet::GetGoldAttribute(), EGameplayModOp::Additive, -CombinePrice);

	if ( OutTransaction )
	{
		OutTransaction->GoldAmount = CombinePrice;
		OutTransaction->ConsumedIngredients.Reset();
	}

	for ( UInventoryItem* Ingredient : OwnedIngredients )
	{
		if ( OutTransaction )
		{
			OutTransaction->ConsumedIngredients.Emplace(Ingredient->GetShopItem(), Ingredient->GetStackCount());
		}

		RemoveItem(Ingredient);
	}

	bool bCreatedNewEntry = false;
	const FInventoryItemHandle GrantedHandle = GrantItem(ItemToCombine, &bCreatedNewEntry);

	if ( OutTransaction )
	{
		OutTransaction->ItemHandle = GrantedHandle;
		OutTransaction->bCreatedNewEntry = bCreatedNewEntry;
	}

	return true;
}

float UInventoryComponent::GetEffectivePrice(const UPA_ShopItem* Item, TArray<UInventoryItem*>* OutOwnedIngredients) const
{
	if ( !Item )
		return 0.f;

	float Price = Item->GetPrice();
	TArray<UInventoryItem*> Used;

	for ( const TSoftObjectPtr<UPA_ShopItem>& IngredientPtr : Item->GetIngredientItems() )
	{
		const UPA_ShopItem* Ingredient = IngredientPtr.LoadSynchronous();
		if ( !Ingredient )
			continue;

		for ( const TPair<FInventoryItemHandle, UInventoryItem*>& Pair : InventoryMap )
		{
			UInventoryItem* InvItem = Pair.Value;
			if ( InvItem && InvItem->IsForItem(Ingredient) && !Used.Contains(InvItem) )
			{
				Used.Add(InvItem);
				Price -= Ingredient->GetPrice();
				break;
			}
		}
	}

	if ( OutOwnedIngredients )
		*OutOwnedIngredients = Used;

	return FMath::Max(Price, 0.f);
}

EPurchaseResult UInventoryComponent::EvaluatePurchase(const UPA_ShopItem* Item, bool bCheckInBase) const
{
	if ( !Item || !OwnerAbilitySystemComponent )
		return EPurchaseResult::InvalidItem;

	if ( bCheckInBase && !OwnerAbilitySystemComponent->HasMatchingGameplayTag(UCAbilitySystemStatics::GetInBaseStatusTag()) )
		return EPurchaseResult::NotInBase;

	if ( Item->GetIsUnique() && TryGetItemForShopItem(Item) != nullptr )
		return EPurchaseResult::AlreadyOwnedUnique;

	TArray<UInventoryItem*> OwnedIngredients;
	const float RequiredGold = GetEffectivePrice(Item, &OwnedIngredients);

	if ( HasExclusiveCategoryConflict(Item, OwnedIngredients) )
		return EPurchaseResult::CategoryLimitReached;

	if ( GetGold() < RequiredGold )
		return EPurchaseResult::NotEnoughGold;

	if ( OwnedIngredients.Num() == 0 && IsFullFor(Item) )
		return EPurchaseResult::InventoryFull;

	return EPurchaseResult::Success;
}

const UPA_ShopItem* UInventoryComponent::FindFallbackIngredientPurchase(const UPA_ShopItem* Item) const
{
	if ( !Item )
		return nullptr;

	// 보유 아이템을 "아이템별 개수"로 집계 (조합 트리 보유 표시와 같은 규칙)
	TMap<const UPA_ShopItem*, int32> AvailableCounts;
	for ( const TPair<FInventoryItemHandle, UInventoryItem*>& Pair : InventoryMap )
	{
		if ( Pair.Value && Pair.Value->GetShopItem() )
		{
			AvailableCounts.FindOrAdd(Pair.Value->GetShopItem())++;
		}
	}

	const UPA_ShopItem* Current = Item;

	// 잘못된(순환) 조합 데이터로 인한 무한 루프 방지
	constexpr int32 MaxDepth = 8;

	for ( int32 Depth = 0; Current && Depth < MaxDepth; ++Depth )
	{
		// 이 단계의 재료를 왼쪽부터: 보유분은 소모 처리, 첫 번째 미보유 재료를 기억
		const UPA_ShopItem* FirstMissing = nullptr;

		for ( const TSoftObjectPtr<UPA_ShopItem>& IngredientPtr : Current->GetIngredientItems() )
		{
			const UPA_ShopItem* Ingredient = IngredientPtr.LoadSynchronous();
			if ( !Ingredient )
				continue;

			int32* RemainingCount = AvailableCounts.Find(Ingredient);
			if ( RemainingCount && *RemainingCount > 0 )
			{
				--(*RemainingCount);
				continue;
			}

			if ( !FirstMissing )
			{
				FirstMissing = Ingredient;
			}
		}

		// 재료를 전부 보유 중인데 골드가 부족한 경우 → 대체 구매 없음
		if ( !FirstMissing )
			return nullptr;

		const EPurchaseResult Result = EvaluatePurchase(FirstMissing);
		if ( Result == EPurchaseResult::Success )
			return FirstMissing;

		// 그 재료도 골드가 부족하면 그 재료의 하위 트리로 내려감. 다른 사유(인벤토리 가득 등)면 중단
		if ( Result != EPurchaseResult::NotEnoughGold )
			return nullptr;

		Current = FirstMissing;
	}

	return nullptr;
}

// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerAbilitySystemComponent = nullptr;

	if ( UAbilitySystemComponent* OwnerASC = GetOwnerASC() )
	{
		OwnerASC->AbilityCommittedCallbacks.AddUObject(this, &UInventoryComponent::AbilityCommitted);
		OwnerASC->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &UInventoryComponent::HandleActiveEffectAdded);

		if ( GetOwner() && GetOwner()->HasAuthority() )
		{
			OwnerASC->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetInBaseStatusTag(), EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UInventoryComponent::InBaseTagUpdated);
			OwnerASC->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetDeadStatusTag(), EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UInventoryComponent::DeadTagUpdated);
		}
	}
}

UAbilitySystemComponent* UInventoryComponent::GetOwnerASC()
{
	if ( !OwnerAbilitySystemComponent || OwnerAbilitySystemComponent->GetOwner() != GetOwner() )
	{
		OwnerAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	}

	return OwnerAbilitySystemComponent;
}

void UInventoryComponent::HandleReplicatedEntryAdded(const FInventoryEntry& Entry)
{
	if ( IsTemplate() )
	{
		ensureMsgf(false, TEXT("[Inventory] Replication callback reached template component: %s"), *GetPathName());
		return;
	}

	if ( !Entry.ShopItem || InventoryMap.Contains(Entry.Handle) )
		return;

	UInventoryItem* InventoryItem = NewObject<UInventoryItem>(this);
	InventoryItem->InitItem(Entry.Handle, Entry.ShopItem, GetOwnerASC());
	InventoryItem->SetStackCount(Entry.StackCount);
	InventoryItem->SetGrantedAbilitySpecHandle(Entry.GrantedAbilitySpecHandle);

	InventoryMap.Add(Entry.Handle, InventoryItem);
	OnItemAdded.Broadcast(InventoryItem);
}

void UInventoryComponent::HandleReplicatedEntryChanged(const FInventoryEntry& Entry)
{
	if ( IsTemplate() )
		return;

	UInventoryItem* InventoryItem = GetInventoryItemByHandle(Entry.Handle);
	if ( !InventoryItem )
	{
		HandleReplicatedEntryAdded(Entry);
		return;
	}

	InventoryItem->SetGrantedAbilitySpecHandle(Entry.GrantedAbilitySpecHandle);

	if ( InventoryItem->GetStackCount() != Entry.StackCount && InventoryItem->SetStackCount(Entry.StackCount) )
	{
		OnItemStackCountChanged.Broadcast(Entry.Handle, Entry.StackCount);
	}
}

void UInventoryComponent::HandleReplicatedEntryRemoved(const FInventoryEntry& Entry)
{
	if ( IsTemplate() )
		return;

	UInventoryItem* InventoryItem = GetInventoryItemByHandle(Entry.Handle);
	if ( !InventoryItem )
		return;

	InventoryItem->RemoveGASModifications();

	OnItemRemoved.Broadcast(Entry.Handle);
	InventoryMap.Remove(Entry.Handle);
}

void UInventoryComponent::AbilityCommitted(class UGameplayAbility* CommittedAbility)
{
	if (!CommittedAbility)
		return;

	float CooldownTimeRemaining = 0.f;
	float CooldownDuration = 0.f;

	CommittedAbility->GetCooldownTimeRemainingAndDuration(
		CommittedAbility->GetCurrentAbilitySpecHandle(),
		CommittedAbility->GetCurrentActorInfo(),
		CooldownTimeRemaining,
		CooldownDuration
	);

	for (TPair<FInventoryItemHandle, UInventoryItem*>& ItemPair : InventoryMap)
	{
		if (!ItemPair.Value)
			continue;

		if (ItemPair.Value->IsGrantingAbility(CommittedAbility->GetClass()))
		{
			OnItemAbilityCommitted.Broadcast(ItemPair.Key, CooldownDuration, CooldownTimeRemaining);
		}
	}
}

void UInventoryComponent::HandleActiveEffectAdded(UAbilitySystemComponent* TargetASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle ActiveHandle)
{
	if ( !OnItemAbilityCommitted.IsBound() )
		return;

	FGameplayTagContainer GrantedTags;
	Spec.GetAllGrantedTags(GrantedTags);

	if ( GrantedTags.IsEmpty() )
		return;

	for ( const TPair<FInventoryItemHandle, UInventoryItem*>& ItemPair : InventoryMap )
	{
		UInventoryItem* Item = ItemPair.Value;
		if ( !Item || !Item->IsGrantingAnyAbility() )
			continue;

		const UGameplayAbility* AbilityCDO = Item->GetShopItem()->GetGrantedAbilityCDO();
		const FGameplayTagContainer* CooldownTags = AbilityCDO ? AbilityCDO->GetCooldownTags() : nullptr;

		if ( !CooldownTags || CooldownTags->IsEmpty() || !GrantedTags.HasAny(*CooldownTags) )
			continue;

		const float Duration = Spec.GetDuration();
		float Remaining = Item->GetAbilityCooldownTimeRemaining();
		if ( Remaining <= 0.f )
		{
			Remaining = Duration;
		}

		OnItemAbilityCommitted.Broadcast(ItemPair.Key, Duration, Remaining);
	}
}

bool UInventoryComponent::HasExclusiveCategoryConflict(const UPA_ShopItem* Item, const TArray<UInventoryItem*>& ItemsToBeConsumed) const
{
	const UCGameplayEffectSettings* Settings = GetDefault<UCGameplayEffectSettings>();
	if (!Settings || !Item)
		return false;

	FGameplayTagContainer ItemExclusiveCategories = Item->GetItemCategories().Filter(Settings->ExclusiveItemCategories);
	if (ItemExclusiveCategories.IsEmpty())
		return false;

	for (const TPair<FInventoryItemHandle, UInventoryItem*>& ItemPair : InventoryMap)
	{
		UInventoryItem* OwnedItem = ItemPair.Value;
		if (!OwnedItem || ItemsToBeConsumed.Contains(OwnedItem))
			continue;

		const UPA_ShopItem* OwnedShopItem = OwnedItem->GetShopItem();
		if (OwnedShopItem && OwnedShopItem->GetItemCategories().HasAny(ItemExclusiveCategories))
			return true;
	}

	return false;
}

void UInventoryComponent::Server_ActivateItem_Implementation(FInventoryItemHandle ItemHandle)
{
	if ( !ACGameState::IsMatchInProgress(this) )
		return;

	if ( !CheckServerRequestRate() )
		return;

	UInventoryItem* InventoryItem = GetInventoryItemByHandle(ItemHandle);
	if ( !InventoryItem || !InventoryItem->IsValid() )
		return;

	if ( UCAbilitySystemStatics::IsActorDead(GetOwner()) )
		return;

	if ( InventoryItem->IsGrantingAnyAbility() && !InventoryItem->TryActivateGrantedAbility() )
		return;

	const bool bIsConsumable = InventoryItem->GetShopItem()->GetIsConsumable();
	if ( !InventoryItem->IsGrantingAnyAbility() && !bIsConsumable )
		return;  

	ClearTransactionHistory();

	if ( bIsConsumable )
	{
		ConsumeItem(InventoryItem);
	}
}

bool UInventoryComponent::Server_ActivateItem_Validate(FInventoryItemHandle ItemHandle)
{
	return ItemHandle.IsValid();
}

void UInventoryComponent::Server_SellItem_Implementation(FInventoryItemHandle ItemHandle)
{
	if ( !ACGameState::IsMatchInProgress(this) )
		return;

	if (!GetOwner()->HasAuthority())
		return;

	if ( !CheckServerRequestRate() )
		return;

	if ( !OwnerAbilitySystemComponent || !OwnerAbilitySystemComponent->HasMatchingGameplayTag(UCAbilitySystemStatics::GetInBaseStatusTag()) )
	{
		Client_SellFailed();
		return;
	}

	UInventoryItem* InventoryItem = GetInventoryItemByHandle(ItemHandle);
	if (!InventoryItem || !InventoryItem->IsValid())
	{
		Client_SellFailed();
		return;
	}

	const UPA_ShopItem* SoldShopItem = InventoryItem->GetShopItem();
	const float SellPrice = SoldShopItem->GetSellPrice();

	OwnerAbilitySystemComponent->ApplyModToAttribute(UCHeroAttributeSet::GetGoldAttribute(), EGameplayModOp::Additive, SellPrice);

	FInventoryTransaction Transaction;
	Transaction.Type = EInventoryTransactionType::Sell;
	Transaction.ShopItem = SoldShopItem;
	Transaction.ItemHandle = InventoryItem->GetHandle();
	Transaction.GoldAmount = SellPrice;

	if ( SoldShopItem->GetIsStackable() && InventoryItem->ReduceStackCount() )
	{
		InventoryList.UpdateStackCount(InventoryItem->GetHandle(), InventoryItem->GetStackCount());
		OnItemStackCountChanged.Broadcast(InventoryItem->GetHandle(), InventoryItem->GetStackCount());

		SyncScoreboardItems();
		Transaction.bRemovedEntry = false;
	}
	else
	{
		RemoveItem(InventoryItem);
		Transaction.bRemovedEntry = true;
	}

	PushTransaction(Transaction);

	Client_ItemSold(SoldShopItem);
}

bool UInventoryComponent::Server_SellItem_Validate(FInventoryItemHandle ItemHandle)
{
	return ItemHandle.IsValid();
}

FInventoryItemHandle UInventoryComponent::GrantItem(const UPA_ShopItem* NewItem, bool* bOutCreatedNewEntry)
{
	if ( bOutCreatedNewEntry )
		*bOutCreatedNewEntry = false;

	if ( !GetOwner()->HasAuthority() || !NewItem )
		return FInventoryItemHandle::InvalidHandle();

	if ( UInventoryItem* StackItem = GetAvailableStackForItem(NewItem) )
	{
		StackItem->AddStackCount();
		InventoryList.UpdateStackCount(StackItem->GetHandle(), StackItem->GetStackCount());
		OnItemStackCountChanged.Broadcast(StackItem->GetHandle(), StackItem->GetStackCount());

		SyncScoreboardItems();

		return StackItem->GetHandle();
	}

	UInventoryItem* InventoryItem = NewObject<UInventoryItem>(this);
	FInventoryItemHandle NewHandle = FInventoryItemHandle::CreateHandle();

	InventoryItem->InitItem(NewHandle, NewItem, OwnerAbilitySystemComponent);
	InventoryMap.Add(NewHandle, InventoryItem);
	InventoryList.AddEntry(*InventoryItem);
	OnItemAdded.Broadcast(InventoryItem);

	SyncScoreboardItems();

	if ( bOutCreatedNewEntry )
		*bOutCreatedNewEntry = true;

	return NewHandle;
}

void UInventoryComponent::ConsumeItem(UInventoryItem* Item)
{
	if (!GetOwner()->HasAuthority())
		return;

	if (!Item)
		return;

	Item->ApplyConsumeEffect();
	if (!Item->ReduceStackCount())
	{
		RemoveItem(Item);
	}
	else
	{
		InventoryList.UpdateStackCount(Item->GetHandle(), Item->GetStackCount());

		OnItemStackCountChanged.Broadcast(Item->GetHandle(), Item->GetStackCount());

		SyncScoreboardItems();
	}
}

void UInventoryComponent::RemoveItem(UInventoryItem* Item)
{
	if (!GetOwner()->HasAuthority())
		return;

	Item->RemoveGASModifications();

	OnItemRemoved.Broadcast(Item->GetHandle());

	InventoryMap.Remove(Item->GetHandle());
	InventoryList.RemoveEntry(Item->GetHandle());

	SyncScoreboardItems();
}

void UInventoryComponent::Client_PurchaseFailed_Implementation(EPurchaseResult Reason, const UPA_ShopItem* Item)
{
	OnPurchaseFailed.Broadcast(Reason, Item);
}

void UInventoryComponent::Server_Purchase_Implementation(const UPA_ShopItem* ItemToPurchase)
{
	if ( !ACGameState::IsMatchInProgress(this) )
		return;

	if ( !CheckServerRequestRate() )
		return;

	const UPA_ShopItem* PurchaseTarget = ItemToPurchase;
	EPurchaseResult Result = EvaluatePurchase(PurchaseTarget);

	if ( Result == EPurchaseResult::NotEnoughGold )
	{
		if ( const UPA_ShopItem* FallbackItem = FindFallbackIngredientPurchase(PurchaseTarget) )
		{
			PurchaseTarget = FallbackItem;
			Result = EPurchaseResult::Success;
		}
	}

	if ( Result != EPurchaseResult::Success )
	{
		Client_PurchaseFailed(Result, ItemToPurchase);
		return;
	}

	FInventoryTransaction Transaction;
	Transaction.Type = EInventoryTransactionType::Purchase;
	Transaction.ShopItem = PurchaseTarget;

	if ( !TryCombinePurchase(PurchaseTarget, &Transaction) )
	{
		const float Price = PurchaseTarget->GetPrice();
		OwnerAbilitySystemComponent->ApplyModToAttribute(UCHeroAttributeSet::GetGoldAttribute(), EGameplayModOp::Additive, -Price);

		Transaction.GoldAmount = Price;
		Transaction.ItemHandle = GrantItem(PurchaseTarget, &Transaction.bCreatedNewEntry);
	}

	PushTransaction(Transaction);

	Client_PurchaseSucceeded(PurchaseTarget);
}

bool UInventoryComponent::Server_Purchase_Validate(const UPA_ShopItem* ItemToPurchase)
{
	return true;
}

bool UInventoryComponent::CheckServerRequestRate()
{
	const float Now = GetWorld()->GetTimeSeconds();

	if ( Now - RequestWindowStartTime >= 1.f )
	{
		RequestWindowStartTime = Now;
		RequestCountInWindow = 0;
	}

	if ( ++RequestCountInWindow > MaxServerRequestsPerSecond )
	{
		UE_LOG(LogTemp, Warning, TEXT("[Inventory] %s: too many server requests, ignored"), *GetNameSafe(GetOwner()));
		return false;
	}

	return true;
}

void UInventoryComponent::Client_PurchaseSucceeded_Implementation(const UPA_ShopItem* Item)
{
	OnPurchaseSucceeded.Broadcast(Item);
}

void UInventoryComponent::Client_ItemSold_Implementation(const UPA_ShopItem* Item)
{
	OnItemSold.Broadcast(Item);
}

void UInventoryComponent::Client_SellFailed_Implementation()
{
	OnSellFailed.Broadcast();
}

void UInventoryComponent::Server_UndoLastTransaction_Implementation()
{
	if ( !ACGameState::IsMatchInProgress(this) )
		return;

	if ( !CheckServerRequestRate() )
		return;

	const bool bInBase = OwnerAbilitySystemComponent && OwnerAbilitySystemComponent->HasMatchingGameplayTag(UCAbilitySystemStatics::GetInBaseStatusTag());
	if ( !bInBase || TransactionHistory.Num() == 0 )
	{
		Client_UndoFailed();
		return;
	}

	const FInventoryTransaction Transaction = TransactionHistory.Pop();

	if ( !ApplyUndo(Transaction) )
	{
		// 기록과 현재 인벤토리가 맞지 않으면 나머지 기록도 신뢰할 수 없으므로 전부 비움
		ClearTransactionHistory();
		Client_UndoFailed();
		return;
	}

	UpdateUndoCount();
	Client_UndoSucceeded();
}

bool UInventoryComponent::Server_UndoLastTransaction_Validate()
{
	return true;
}

bool UInventoryComponent::ApplyUndo(const FInventoryTransaction& Transaction)
{
	const UPA_ShopItem* ShopItem = Transaction.ShopItem.Get();
	if ( !ShopItem || !OwnerAbilitySystemComponent )
		return false;

	if ( Transaction.Type == EInventoryTransactionType::Purchase )
	{
		UInventoryItem* PurchasedItem = GetInventoryItemByHandle(Transaction.ItemHandle);
		if ( !PurchasedItem || !PurchasedItem->IsForItem(ShopItem) )
			return false;

		// 산 아이템 제거 (스택에 더해졌던 경우 1개만)
		if ( Transaction.bCreatedNewEntry || !PurchasedItem->ReduceStackCount() )
		{
			RemoveItem(PurchasedItem);
		}
		else
		{
			InventoryList.UpdateStackCount(PurchasedItem->GetHandle(), PurchasedItem->GetStackCount());
			OnItemStackCountChanged.Broadcast(PurchasedItem->GetHandle(), PurchasedItem->GetStackCount());
			SyncScoreboardItems();
		}

		// 조합으로 소모된 재료 복원
		for ( const TPair<TWeakObjectPtr<const UPA_ShopItem>, int32>& Consumed : Transaction.ConsumedIngredients )
		{
			const UPA_ShopItem* IngredientItem = Consumed.Key.Get();
			for ( int32 Count = 0; IngredientItem && Count < Consumed.Value; ++Count )
			{
				GrantItem(IngredientItem);
			}
		}

		// 전액 환불
		OwnerAbilitySystemComponent->ApplyModToAttribute(UCHeroAttributeSet::GetGoldAttribute(), EGameplayModOp::Additive, Transaction.GoldAmount);
		return true;
	}

	// Sell
	if ( GetGold() < Transaction.GoldAmount )
		return false;

	if ( Transaction.bRemovedEntry )
	{
		if ( IsFullFor(ShopItem) )
			return false;

		GrantItem(ShopItem);
	}
	else
	{
		UInventoryItem* SoldItem = GetInventoryItemByHandle(Transaction.ItemHandle);
		if ( !SoldItem || !SoldItem->IsForItem(ShopItem) || !SoldItem->AddStackCount() )
			return false;

		InventoryList.UpdateStackCount(SoldItem->GetHandle(), SoldItem->GetStackCount());
		OnItemStackCountChanged.Broadcast(SoldItem->GetHandle(), SoldItem->GetStackCount());
		SyncScoreboardItems();
	}

	// 판매로 받은 골드 회수
	OwnerAbilitySystemComponent->ApplyModToAttribute(UCHeroAttributeSet::GetGoldAttribute(), EGameplayModOp::Additive, -Transaction.GoldAmount);
	return true;
}

void UInventoryComponent::PushTransaction(const FInventoryTransaction& Transaction)
{
	TransactionHistory.Add(Transaction);

	if ( TransactionHistory.Num() > MaxUndoHistory )
	{
		TransactionHistory.RemoveAt(0, TransactionHistory.Num() - MaxUndoHistory);
	}

	UpdateUndoCount();
}

void UInventoryComponent::ClearTransactionHistory()
{
	if ( TransactionHistory.Num() == 0 )
		return;

	TransactionHistory.Reset();
	UpdateUndoCount();
}

void UInventoryComponent::UpdateUndoCount()
{
	const int32 NewCount = TransactionHistory.Num();
	if ( UndoCount == NewCount )
		return;

	UndoCount = NewCount;

	// 서버(리슨 서버 호스트)는 OnRep이 호출되지 않으므로 직접 알림
	OnUndoAvailabilityChanged.Broadcast(CanUndo());
}

void UInventoryComponent::OnRep_UndoCount()
{
	OnUndoAvailabilityChanged.Broadcast(CanUndo());
}

void UInventoryComponent::InBaseTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
	// 기지를 벗어나면 초기화
	if ( NewCount <= 0 )
	{
		ClearTransactionHistory();
	}
}

void UInventoryComponent::DeadTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
	// 사망하면 초기화
	if ( NewCount > 0 )
	{
		ClearTransactionHistory();
	}
}

void UInventoryComponent::Client_UndoSucceeded_Implementation()
{
	OnUndoSucceeded.Broadcast();
}

void UInventoryComponent::Client_UndoFailed_Implementation()
{
	OnUndoFailed.Broadcast();
}