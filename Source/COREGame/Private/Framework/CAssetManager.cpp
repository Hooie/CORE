// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/CAssetManager.h"
#include "Character/PA_CharacterDefinition.h"
#include "GAS/CGameplayEffectSettings.h"
#include "UObject/UnrealType.h"

UCAssetManager& UCAssetManager::Get()
{
	UCAssetManager* Singleton = Cast<UCAssetManager>(GEngine->AssetManager.Get());

	if ( Singleton )
		return *Singleton;

	UE_LOG(LogLoad, Fatal, TEXT("Asset Manager Needs to be of the type CAssetManager!"));
	return (*NewObject<UCAssetManager>());
}

void UCAssetManager::LoadCharacterDefinitions(const FStreamableDelegate& LoadFinishedCallback)
{
	LoadPrimaryAssetsWithType(UPA_CharacterDefinition::GetCharacterDefinitionAssetType(), TArray<FName>(), LoadFinishedCallback);
}

bool UCAssetManager::GetLoadedCharacterDefinitions(TArray<UPA_CharacterDefinition*>& LoadedCharacterDefinition) const
{
	TArray<UObject*> LoadedObjects;
	bool bLoaded = GetPrimaryAssetObjectList(UPA_CharacterDefinition::GetCharacterDefinitionAssetType(), LoadedObjects);
	if (bLoaded)
	{
		for (UObject* LoadedObject : LoadedObjects)
		{
			LoadedCharacterDefinition.Add(Cast<UPA_CharacterDefinition>(LoadedObject));
		}
	}

	return bLoaded;
}

void UCAssetManager::PreloadLobbyCharacterAssets(const FStreamableDelegate& OnListReady)
{
	LoadPrimaryAssetsWithType(UPA_CharacterDefinition::GetCharacterDefinitionAssetType(), TArray<FName>(),
		FStreamableDelegate::CreateUObject(this, &UCAssetManager::HandleLobbyCharacterDefinitionsLoaded, OnListReady));
}

void UCAssetManager::RequestCharacterDisplayAssets(const UPA_CharacterDefinition* Definition, const FStreamableDelegate& OnLoaded)
{
	if ( !Definition )
		return;

	if ( Definition->IsDisplayAssetsLoaded() )
	{
		OnLoaded.ExecuteIfBound();
		return;
	}

	TArray<FSoftObjectPath> Paths;
	Definition->GetDisplayAssetPaths(Paths);

	GetStreamableManager().RequestAsyncLoad(Paths, OnLoaded, FStreamableManager::AsyncLoadHighPriority, /*bManageActiveHandle*/ true);
}

void UCAssetManager::LoadShopItems(const FStreamableDelegate& LoadFinishedCallback)
{
	LoadPrimaryAssetsWithType(UPA_ShopItem::GetShopItemAssetType(), TArray<FName>(), FStreamableDelegate::CreateUObject(this, &UCAssetManager::ShopItemLoadFinished, LoadFinishedCallback));
}

bool UCAssetManager::GetLoadedShopItems(TArray<const UPA_ShopItem*>& OutItems) const
{
	TArray<UObject*> LoadedObjects;
	bool bLoaded = GetPrimaryAssetObjectList(UPA_ShopItem::GetShopItemAssetType(), LoadedObjects);

	if (bLoaded)
	{
		for (UObject* ObjectLoaded : LoadedObjects)
		{
			OutItems.Add(Cast<UPA_ShopItem>(ObjectLoaded));
		}
	}

	return bLoaded;
}

const FItemCollection* UCAssetManager::GetCombinationForItem(const UPA_ShopItem* Item) const
{
	return CombinationMap.Find(Item);
}

const FItemCollection* UCAssetManager::GetIngredientForItem(const UPA_ShopItem* Item) const
{
	return IngredientMap.Find(Item);
}

void UCAssetManager::PreloadMatchResources(bool bIncludeClientAssets, const FSimpleDelegate& OnFinished)
{
	FCMatchPreloadState& State = GetPreloadState(bIncludeClientAssets);

	if ( State.bFinished )
	{
		OnFinished.ExecuteIfBound();
		return;
	}

	State.PendingCallbacks.Add(OnFinished);

	if ( State.bStarted )
		return;

	State.bStarted = true;

	TArray<FPrimaryAssetId> AssetIds;
	GetPrimaryAssetIdList(UPA_ShopItem::GetShopItemAssetType(), AssetIds);

	if ( bIncludeClientAssets )
	{
		TArray<FPrimaryAssetId> CharacterAssetIds;
		GetPrimaryAssetIdList(UPA_CharacterDefinition::GetCharacterDefinitionAssetType(), CharacterAssetIds);
		AssetIds.Append(CharacterAssetIds);
	}

	State.PrimaryHandle = LoadPrimaryAssets(AssetIds, TArray<FName>(), FStreamableDelegate::CreateUObject(this, &UCAssetManager::HandlePreloadPrimaryLoaded, bIncludeClientAssets));

	if ( !State.PrimaryHandle.IsValid() || State.PrimaryHandle->HasLoadCompleted() )
	{
		HandlePreloadPrimaryLoaded(bIncludeClientAssets);
	}
}

float UCAssetManager::GetMatchPreloadProgress(bool bIncludeClientAssets) const
{
	const FCMatchPreloadState& State = GetPreloadState(bIncludeClientAssets);

	if ( State.bFinished )
		return 1.f;

	if ( !State.bStarted )
		return 0.f;

	const float PrimaryProgress = State.bPrimaryLoaded ? 1.f : (State.PrimaryHandle.IsValid() ? State.PrimaryHandle->GetProgress() : 0.f);
	const float SecondaryProgress = State.SecondaryHandle.IsValid() ? State.SecondaryHandle->GetProgress() : 0.f;

	return FMath::Clamp(PrimaryProgress * 0.4f + SecondaryProgress * 0.6f, 0.f, 1.f);
}

bool UCAssetManager::IsMatchPreloadFinished(bool bIncludeClientAssets) const
{
	return GetPreloadState(bIncludeClientAssets).bFinished;
}

void UCAssetManager::ShopItemLoadFinished(FStreamableDelegate Callback)
{
	BuildItemMaps();

	Callback.ExecuteIfBound();
}

void UCAssetManager::BuildItemMaps()
{
	if ( bItemMapBuilt )
		return;

	TArray<const UPA_ShopItem*> LoadedItems;
	if ( !GetLoadedShopItems(LoadedItems) || LoadedItems.Num() == 0 )
		return;

	bItemMapBuilt = true;
	
	for (const UPA_ShopItem* Item : LoadedItems)
	{
		if ( !Item || Item->GetIngredientItems().Num() == 0)
		{
			continue;
		}

		TArray<const UPA_ShopItem*> Items;
		for (const TSoftObjectPtr<UPA_ShopItem>& Ingredient : Item->GetIngredientItems())
		{
			UPA_ShopItem* IngredientItem = Ingredient.LoadSynchronous();
			Items.Add(IngredientItem);

			AddToCombinationMap(IngredientItem, Item);
		}

		IngredientMap.Add(Item, FItemCollection{ Items });
	}

}

void UCAssetManager::AddToCombinationMap(const UPA_ShopItem* Ingredient, const UPA_ShopItem* CombinationItem)
{
	FItemCollection* Combinations = CombinationMap.Find(Ingredient);
	if (Combinations)
	{
		if (!Combinations->Contains(CombinationItem))
			Combinations->AddItem(CombinationItem);
	}
	else
	{
		CombinationMap.Add(Ingredient, FItemCollection{TArray<const UPA_ShopItem*>{CombinationItem}});
	}
}

FCMatchPreloadState& UCAssetManager::GetPreloadState(bool bIncludeClientAssets)
{
	return bIncludeClientAssets ? ClientPreloadState : ServerPreloadState;
}

const FCMatchPreloadState& UCAssetManager::GetPreloadState(bool bIncludeClientAssets) const
{
	return bIncludeClientAssets ? ClientPreloadState : ServerPreloadState;
}

void UCAssetManager::HandlePreloadPrimaryLoaded(bool bIncludeClientAssets)
{
	FCMatchPreloadState& State = GetPreloadState(bIncludeClientAssets);
	if ( State.bPrimaryLoaded )
		return;

	State.bPrimaryLoaded = true;

	BuildItemMaps();

	TArray<FSoftObjectPath> Paths;
	GatherPreloadPaths(bIncludeClientAssets, Paths);

	if ( Paths.Num() == 0 )
	{
		HandlePreloadFinished(bIncludeClientAssets);
		return;
	}

	State.SecondaryHandle = GetStreamableManager().RequestAsyncLoad(Paths, FStreamableDelegate::CreateUObject(this, &UCAssetManager::HandlePreloadFinished, bIncludeClientAssets));

	if ( !State.SecondaryHandle.IsValid() || State.SecondaryHandle->HasLoadCompleted() )
	{
		HandlePreloadFinished(bIncludeClientAssets);
	}
}

void UCAssetManager::HandlePreloadFinished(bool bIncludeCientAssets)
{
	FCMatchPreloadState& State = GetPreloadState(bIncludeCientAssets);
	if ( State.bFinished )
		return;

	State.bFinished = true;

	UE_LOG(LogTemp, Log, TEXT("[Preload] %s 리소스 사전 로드 완료"), bIncludeCientAssets ? TEXT("클라이언트") : TEXT("서버"));

	TArray<FSimpleDelegate> Callbacks = MoveTemp(State.PendingCallbacks);
	State.PendingCallbacks.Reset();

	for ( const FSimpleDelegate& Callback : Callbacks )
	{
		Callback.ExecuteIfBound();
	}
}

void UCAssetManager::GatherPreloadPaths(bool bIncludeClientAssets, TArray<FSoftObjectPath>& OutPaths) const
{
	// CGameplayEffectSettings의 모든 Soft 참조
	if ( const UCGameplayEffectSettings* Settings = GetDefault<UCGameplayEffectSettings>() )
	{
		for ( TFieldIterator<FSoftObjectProperty> It(UCGameplayEffectSettings::StaticClass()); It; ++It )
		{
			const FSoftObjectPtr* SoftPtr = It->GetPropertyValuePtr_InContainer(Settings);
			if ( SoftPtr && !SoftPtr->IsNull() )
			{
				OutPaths.AddUnique(SoftPtr->ToSoftObjectPath());
			}
		}
	}

	if ( !bIncludeClientAssets )
		return;

	// 상점 아이템 아이콘
	TArray<const UPA_ShopItem*> ShopItems;
	if ( GetLoadedShopItems(ShopItems) )
	{
		for ( const UPA_ShopItem* Item : ShopItems )
		{
			if ( Item && !Item->GetIconSoftPtr().IsNull() )
			{
				OutPaths.AddUnique(Item->GetIconSoftPtr().ToSoftObjectPath());
			}
		}
	}

	// 캐릭터 아이콘 및 클래스
	TArray<UPA_CharacterDefinition*> Definitions;
	if ( GetLoadedCharacterDefinitions(Definitions) )
	{
		for ( const UPA_CharacterDefinition* Definition : Definitions )
		{
			if ( !Definition )
				continue;

			if ( !Definition->GetIconSoftPtr().IsNull() )
			{
				OutPaths.AddUnique(Definition->GetIconSoftPtr().ToSoftObjectPath());
			}

			if ( !Definition->GetCharacterClassSoftPtr().IsNull() )
			{
				OutPaths.AddUnique(Definition->GetCharacterClassSoftPtr().ToSoftObjectPath());
			}
		}
	}
}

void UCAssetManager::HandleLobbyCharacterDefinitionsLoaded(FStreamableDelegate OnListReady)
{
	TArray<UPA_CharacterDefinition*> Definitions;
	GetLoadedCharacterDefinitions(Definitions);

	TArray<FSoftObjectPath> IconPaths;
	for ( const UPA_CharacterDefinition* Definition : Definitions )
	{
		if ( Definition && !Definition->GetIconSoftPtr().IsNull() )
		{
			IconPaths.AddUnique(Definition->GetIconSoftPtr().ToSoftObjectPath());
		}
	}

	if ( IconPaths.Num() == 0 )
	{
		HandleLobbyIconsLoaded(OnListReady);
		return;
	}

	GetStreamableManager().RequestAsyncLoad(IconPaths,
		FStreamableDelegate::CreateUObject(this, &UCAssetManager::HandleLobbyIconsLoaded, OnListReady),
		FStreamableManager::AsyncLoadHighPriority, /*bManageActiveHandle*/ true);
}

void UCAssetManager::HandleLobbyIconsLoaded(FStreamableDelegate OnListReady)
{
	OnListReady.ExecuteIfBound();

	if ( LobbyDisplayAssetsHandle.IsValid() )
		return;

	TArray<UPA_CharacterDefinition*> Definitions;
	GetLoadedCharacterDefinitions(Definitions);

	TArray<FSoftObjectPath> DisplayPaths;
	for ( const UPA_CharacterDefinition* Definition : Definitions )
	{
		if ( Definition )
		{
			Definition->GetDisplayAssetPaths(DisplayPaths);
		}
	}

	if ( DisplayPaths.Num() > 0 )
	{
		LobbyDisplayAssetsHandle = GetStreamableManager().RequestAsyncLoad(DisplayPaths, FStreamableDelegate());
	}
}
