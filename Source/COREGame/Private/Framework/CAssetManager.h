// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "Inventory/PA_ShopItem.h"

#include "CAssetManager.generated.h"

class UPA_CharacterDefinition;

// 게임 리소스 사전 로드 상태 (서버 / 클라 각각 하나씩)
struct FCMatchPreloadState
{
	TSharedPtr<FStreamableHandle> PrimaryHandle;	// 상점 아이템, 캐릭터 Definition
	TSharedPtr<FStreamableHandle> SecondaryHandle;	// 아이콘, 캐릭터 클래스, 아이템 GE

	bool bStarted = false;
	bool bPrimaryLoaded = false;
	bool bFinished = false;
	TArray<FSimpleDelegate> PendingCallbacks;
};

/**
 * 
 */
UCLASS()
class UCAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	static UCAssetManager& Get();

	void LoadCharacterDefinitions(const FStreamableDelegate& LoadFinishedCallback);
	bool GetLoadedCharacterDefinitions(TArray<UPA_CharacterDefinition*>& LoadedCharacterDefinition) const;
	
	void PreloadLobbyCharacterAssets(const FStreamableDelegate& OnListReady);
	void RequestCharacterDisplayAssets(const UPA_CharacterDefinition* Definition, const FStreamableDelegate& OnLoaded);

	void LoadShopItems(const FStreamableDelegate& LoadFinishedCallback);
	bool GetLoadedShopItems(TArray<const UPA_ShopItem*>& OutItems) const;

	const FItemCollection* GetCombinationForItem(const UPA_ShopItem* Item) const;
	const FItemCollection* GetIngredientForItem(const UPA_ShopItem* Item) const;

	void PreloadMatchResources(bool bIncludeClientAssets, const FSimpleDelegate& OnFinished);
	float GetMatchPreloadProgress(bool bIncludeClientAssets) const;
	bool IsMatchPreloadFinished(bool bIncludeClientAssets) const;

private:
	void ShopItemLoadFinished(FStreamableDelegate Callback);
	void BuildItemMaps();
	void AddToCombinationMap(const UPA_ShopItem* Ingredient, const UPA_ShopItem* CombinationItem);

	UPROPERTY()
	TMap<const UPA_ShopItem*, FItemCollection> CombinationMap;

	UPROPERTY()
	TMap<const UPA_ShopItem*, FItemCollection> IngredientMap;

	bool bItemMapBuilt = false;

	FCMatchPreloadState ServerPreloadState;		// 상점 아이템 + 아이템 GE
	FCMatchPreloadState ClientPreloadState;		// 상점 아이템 + 상점 아이콘 + 아이템 GE + 캐릭터 클래스 + 캐릭터 아이콘

	FCMatchPreloadState& GetPreloadState(bool bIncludeClientAssets);
	const FCMatchPreloadState& GetPreloadState(bool bIncludeClientAssets) const;

	void HandlePreloadPrimaryLoaded(bool bIncludeClientAssets);
	void HandlePreloadFinished(bool bIncludeCientAssets);
	void GatherPreloadPaths(bool bIncludeClientAssets, TArray<FSoftObjectPath>& OutPaths) const;

	void HandleLobbyCharacterDefinitionsLoaded(FStreamableDelegate OnListReady);
	void HandleLobbyIconsLoaded(FStreamableDelegate OnListReady);

	TSharedPtr<FStreamableHandle> LobbyDisplayAssetsHandle;
};
