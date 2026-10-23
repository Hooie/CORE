// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GAS/CGameplayAbilityTypes.h"
#include "PA_CharacterDefinition.generated.h"

class ACCharacter;
class UGameplayAbility;

/**
 * 
 */
UCLASS()
class UPA_CharacterDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	static FPrimaryAssetType GetCharacterDefinitionAssetType();

	FString GetCharacterDisplayName() const { return CharacterName; }
	UTexture2D* LoadIcon() const;
	const TSoftObjectPtr<UTexture2D>& GetIconSoftPtr() const { return CharacterIcon; }
	const TSoftClassPtr<ACCharacter>& GetCharacterClassSoftPtr() const { return CharacterClass; }

	const TSoftObjectPtr<USkeletalMesh>& GetDisplayMeshSoftPtr() const { return DisplayMesh; }
	const TSoftClassPtr<UAnimInstance>& GetDisplayAnimBPSoftPtr() const { return DisplayAnimBP; }

	bool IsDisplayAssetsLoaded() const;
	void GetDisplayAssetPaths(TArray<FSoftObjectPath>& OutPaths) const;

	TSubclassOf<ACCharacter> LoadCharacterClass() const;
	TSubclassOf<UAnimInstance> LoadDisplayAnimBP() const;
	TSoftObjectPtr<USkeletalMesh> LoadDisplayMesh() const;

	const TMap<ECAbilityInputID, TSubclassOf<UGameplayAbility>>* GetAbilities() const;
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "Character")
	FString CharacterName;

	UPROPERTY(EditDefaultsOnly, Category = "Character")
	TSoftObjectPtr<UTexture2D> CharacterIcon;

	UPROPERTY(EditDefaultsOnly, Category = "Character")
	TSoftClassPtr<ACCharacter> CharacterClass;

	UPROPERTY(EditDefaultsOnly, Category = "Character")
	TSoftClassPtr<UAnimInstance> DisplayAnimBP;

	UPROPERTY(EditDefaultsOnly, Category = "Character")
	TSoftObjectPtr<USkeletalMesh> DisplayMesh;
};
