// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/InventoryItem.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "InventoryComponent.generated.h"

class UAbilitySystemComponent;
class UPA_ShopItem;
class UInventoryComponent;
struct FInventoryList;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemAddedDelegate, const UInventoryItem* /*New Item*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnItemStackCountChangeDelegate, const FInventoryItemHandle&, int /*New Count*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemRemovedDelegate, const FInventoryItemHandle& /*Item Handle*/);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnItemAbilityCommittedDelegate, const FInventoryItemHandle&, float /*Cooldown Duration*/, float /*Cooldown Remaining*/);


UENUM(BlueprintType)
enum class EPurchaseResult : uint8
{
	Success,
	InvalidItem,
	NotInBase,
	AlreadyOwnedUnique,
	CategoryLimitReached,
	NotEnoughGold,
	InventoryFull
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPurchaseFailedDelegate, EPurchaseResult /*Purchase Result*/, const UPA_ShopItem* /*Item to Purchase*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPurchaseSucceededDelegate, const UPA_ShopItem* /*Item to Sell*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnItemSoldDelegate, const UPA_ShopItem* /*Item Sold*/);
DECLARE_MULTICAST_DELEGATE(FOnSellFailedDelegate);

/************************************************************************/
/**************************Replicated Inventory**************************/
/************************************************************************/

/* 서버 -> 클라이언트로 복제되는 인벤토리 항목 1개 */
USTRUCT()
struct FInventoryEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY()
	FInventoryItemHandle Handle;

	UPROPERTY()
	const UPA_ShopItem* ShopItem = nullptr;

	UPROPERTY()
	int32 StackCount = 1;

	UPROPERTY()
	FGameplayAbilitySpecHandle GrantedAbilitySpecHandle;

	// 클라이언트에서 복제 수신 시 호출됨
	void PreReplicatedRemove(const FInventoryList& InArray);
	void PostReplicatedAdd(const FInventoryList& InArray);
	void PostReplicatedChange(const FInventoryList& InArray);
};

USTRUCT()
struct FInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()

	FInventoryList() = default;

	// BP 템플릿 컴포넌트에서 값이 복사될 때 OwnerComponent까지 덮어써지지 않도록 직접 정의
	FInventoryList(const FInventoryList& Other)
		: FFastArraySerializer(Other)
		, Entries(Other.Entries)
	{
	}

	FInventoryList& operator=(const FInventoryList& Other)
	{
		FFastArraySerializer::operator=(Other);
		Entries = Other.Entries;
		// OwnerComponent는 의도적으로 복사하지 않음
		return *this;
	}

	UPROPERTY()
	TArray<FInventoryEntry> Entries;

	// 복제 X, 복사 X. 컴포넌트의 PostInitProperties에서 설정
	UInventoryComponent* OwnerComponent = nullptr;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FInventoryEntry, FInventoryList>(Entries, DeltaParams, *this);
	}

	// 서버 전용
	void AddEntry(const UInventoryItem& Item);
	void RemoveEntry(const FInventoryItemHandle& Handle);
	void UpdateStackCount(const FInventoryItemHandle& Handle, int32 NewStackCount);
};

template<>
struct TStructOpsTypeTraits<FInventoryList> : public TStructOpsTypeTraitsBase2<FInventoryList>
{
	enum { WithNetDeltaSerializer = true };
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnUndoAvailabilityChangedDelegate, bool /*bCanUndo*/);
DECLARE_MULTICAST_DELEGATE(FOnUndoSucceededDelegate);
DECLARE_MULTICAST_DELEGATE(FOnUndoFailedDelegate);

/************************************************************************/
/***********************Purchase / Sell Undo History*********************/
/************************************************************************/

enum class EInventoryTransactionType : uint8
{
	Purchase,
	Sell
};

/** 서버 전용: 되돌리기용 거래 기록 1건 */
struct FInventoryTransaction
{
	EInventoryTransactionType Type = EInventoryTransactionType::Purchase;

	TWeakObjectPtr<const UPA_ShopItem> ShopItem;

	// 구매: 아이템이 들어간 칸 / 판매: 판매한 칸
	FInventoryItemHandle ItemHandle;

	// 구매: 새 칸을 만들었는지(false면 기존 스택 +1)
	bool bCreatedNewEntry = false;

	// 판매: 칸 자체가 사라졌는지(false면 스택 -1)
	bool bRemovedEntry = false;

	// 구매: 지불한 골드 / 판매: 받은 골드
	float GoldAmount = 0.f;

	// 조합 구매로 소모된 재료 (아이템, 스택 수)
	TArray<TPair<TWeakObjectPtr<const UPA_ShopItem>, int32>> ConsumedIngredients;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

	friend struct FInventoryEntry;

public:
	// Sets default values for this component's properties
	UInventoryComponent();
	virtual void PostInitProperties() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	FOnItemAddedDelegate OnItemAdded;
	FOnItemStackCountChangeDelegate OnItemStackCountChanged;
	FOnItemRemovedDelegate OnItemRemoved;
	FOnItemAbilityCommittedDelegate OnItemAbilityCommitted;

	FOnPurchaseFailedDelegate OnPurchaseFailed;
	FOnPurchaseSucceededDelegate OnPurchaseSucceeded;
	FOnItemSoldDelegate OnItemSold;
	FOnSellFailedDelegate OnSellFailed;

	FOnUndoAvailabilityChangedDelegate OnUndoAvailabilityChanged;
	FOnUndoSucceededDelegate OnUndoSucceeded;
	FOnUndoFailedDelegate OnUndoFailed;

	void TryUndo();
	FORCEINLINE bool CanUndo() const { return UndoCount > 0; }

	void TryActivateItem(const FInventoryItemHandle& ItemHandle);
	void TryPurchase(const UPA_ShopItem* ItemToPurchase);
	void SellItem(const FInventoryItemHandle& ItemHandle);

	bool CanPurchase(const UPA_ShopItem* Item) const;

	float GetGold() const;
	FORCEINLINE int GetCapacity() const { return Capacity; }

	void ItemSlotChanged(const FInventoryItemHandle& Handle, int NewSlotNumber);
	UInventoryItem* GetInventoryItemByHandle(const FInventoryItemHandle& Handle) const;

	bool IsFullFor(const UPA_ShopItem* Item) const;

	bool IsAllSlotOccupied() const;
	UInventoryItem* GetAvailableStackForItem(const UPA_ShopItem* Item) const;

	bool FindIngredientForItem(const UPA_ShopItem* Item, TArray<UInventoryItem*>& OutIngredients, const TArray<const UPA_ShopItem*>& IngredientToIgnore = TArray<const UPA_ShopItem*>{});
	UInventoryItem* TryGetItemForShopItem(const UPA_ShopItem* Item) const;

	void TryActivateItemInSlot(int SlotNumber);

	TArray<UInventoryItem*> GetAllItems() const;

	void SyncScoreboardItems();

	bool TryCombinePurchase(const UPA_ShopItem* ItemToCombine, FInventoryTransaction* OutTransaction = nullptr);

	float GetEffectivePrice(const UPA_ShopItem* Item, TArray<UInventoryItem*>* OutOwnedIngredients = nullptr) const;

	EPurchaseResult EvaluatePurchase(const UPA_ShopItem* Item, bool bCheckInBase = true) const;

	const UPA_ShopItem* FindFallbackIngredientPurchase(const UPA_ShopItem* Item) const;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Inventory")
	int Capacity = 6;

	UPROPERTY(Transient)
	UAbilitySystemComponent* OwnerAbilitySystemComponent;

	// 클라이언트에서는 복제가 BeginPlay보다 먼저 도착할 수 있으므로 지연 조회
	UAbilitySystemComponent* GetOwnerASC();

	// 서버/클라이언트 공통 로컬 뷰 (위젯이 참조하는 UInventoryItem)
	UPROPERTY(Transient)
	TMap<FInventoryItemHandle, UInventoryItem*> InventoryMap;

	// 서버가 진실, 소유 클라이언트로만 복제
	UPROPERTY(Replicated, Transient)
	FInventoryList InventoryList;

	// 클라이언트: 복제 수신 → 로컬 뷰 갱신 + 기존 델리게이트 브로드캐스트
	void HandleReplicatedEntryAdded(const FInventoryEntry& Entry);
	void HandleReplicatedEntryChanged(const FInventoryEntry& Entry);
	void HandleReplicatedEntryRemoved(const FInventoryEntry& Entry);

	void AbilityCommitted(class UGameplayAbility* CommittedAbility);

	void HandleActiveEffectAdded(UAbilitySystemComponent* TargetASC, const FGameplayEffectSpec& Spec, FActiveGameplayEffectHandle ActiveHandle);

	bool HasExclusiveCategoryConflict(const UPA_ShopItem* Item, const TArray<UInventoryItem*>& ItemsToBeConsumed) const;

	/***********************************************************************/
	/*                              Server                                 */
	/***********************************************************************/
private:
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Purchase(const UPA_ShopItem* ItemToPurchase);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ActivateItem(FInventoryItemHandle ItemHandle);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SellItem(FInventoryItemHandle ItemHandle);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UndoLastTransaction();

	FInventoryItemHandle GrantItem(const UPA_ShopItem* NewItem, bool* bOutCreatedNewEntry = nullptr);

	void ConsumeItem(UInventoryItem* Item);
	void RemoveItem(UInventoryItem* Item);

	// 서버 요청 속도 제한 (초당 최대 요청 수)
	UPROPERTY(EditDefaultsOnly, Category = "Inventory|Network")
	int32 MaxServerRequestsPerSecond = 20;

	float RequestWindowStartTime = 0.f;
	int32 RequestCountInWindow = 0;

	bool CheckServerRequestRate();

	UPROPERTY(EditDefaultsOnly, Category = "Inventory|Undo")
	int32 MaxUndoHistory = 10;

	// 서버 전용 기록 (복제 X)
	TArray<FInventoryTransaction> TransactionHistory;

	// 클라이언트에는 개수만 복제 → 되돌리기 버튼 활성화용
	UPROPERTY(ReplicatedUsing = OnRep_UndoCount)
	int32 UndoCount = 0;

	UFUNCTION()
	void OnRep_UndoCount();

	void PushTransaction(const FInventoryTransaction& Transaction);
	void ClearTransactionHistory();
	void UpdateUndoCount();

	bool ApplyUndo(const FInventoryTransaction& Transaction);

	void InBaseTagUpdated(const FGameplayTag Tag, int32 NewCount);
	void DeadTagUpdated(const FGameplayTag Tag, int32 NewCount);

	/***********************************************************************/
	/*                              Client                                 */
	/***********************************************************************/
private:
	UFUNCTION(Client, Reliable)
	void Client_PurchaseFailed(EPurchaseResult Reason, const UPA_ShopItem* Item);

	UFUNCTION(Client, Reliable)
	void Client_PurchaseSucceeded(const UPA_ShopItem* Item);

	UFUNCTION(Client, Reliable)
	void Client_ItemSold(const UPA_ShopItem* Item);

	UFUNCTION(Client, Reliable)
	void Client_SellFailed();

	UFUNCTION(Client, Reliable)
	void Client_UndoSucceeded();

	UFUNCTION(Client, Reliable)
	void Client_UndoFailed();
};