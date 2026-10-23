// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PA_CharacterDefinition.h"
#include "Character/CCharacter.h"

FPrimaryAssetId UPA_CharacterDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(GetCharacterDefinitionAssetType(), GetFName());
}

FPrimaryAssetType UPA_CharacterDefinition::GetCharacterDefinitionAssetType()
{
	return FPrimaryAssetType("CharacterDefinition");
}

UTexture2D* UPA_CharacterDefinition::LoadIcon() const
{
	CharacterIcon.LoadSynchronous();
	if (CharacterIcon.IsValid())
		return CharacterIcon.Get();
	
	return nullptr;
}

bool UPA_CharacterDefinition::IsDisplayAssetsLoaded() const
{
	const bool bMeshReady = DisplayMesh.IsNull() || DisplayMesh.IsValid();
	const bool bAnimReady = DisplayAnimBP.IsNull() || DisplayAnimBP.IsValid();

	return bMeshReady && bAnimReady;
}

void UPA_CharacterDefinition::GetDisplayAssetPaths(TArray<FSoftObjectPath>& OutPaths) const
{
	if ( !DisplayMesh.IsNull() )
	{
		OutPaths.AddUnique(DisplayMesh.ToSoftObjectPath());
	}

	if ( !DisplayAnimBP.IsNull() )
	{
		OutPaths.AddUnique(DisplayAnimBP.ToSoftObjectPath());
	}
}

TSubclassOf<ACCharacter> UPA_CharacterDefinition::LoadCharacterClass() const
{
	CharacterClass.LoadSynchronous();
	if (CharacterClass.IsValid())
		return CharacterClass.Get();

	return TSubclassOf<ACCharacter>();
}

TSubclassOf<UAnimInstance> UPA_CharacterDefinition::LoadDisplayAnimBP() const
{
	DisplayAnimBP.LoadSynchronous();
	if (DisplayAnimBP.IsValid())
		return DisplayAnimBP.Get();

	return TSubclassOf<UAnimInstance>();
}

TSoftObjectPtr<USkeletalMesh> UPA_CharacterDefinition::LoadDisplayMesh() const
{
	DisplayMesh.LoadSynchronous();
	if (DisplayMesh.IsValid())
		return DisplayMesh.Get();

	return TSoftObjectPtr<USkeletalMesh>();
}

const TMap<ECAbilityInputID, TSubclassOf<UGameplayAbility>>* UPA_CharacterDefinition::GetAbilities() const
{
	TSubclassOf<ACCharacter> LoadedCharacterClass = LoadCharacterClass();
	ACCharacter* Character = Cast<ACCharacter>(LoadedCharacterClass.GetDefaultObject());
	if (!Character)
		return nullptr;

	return &(Character->GetAbilities());
}
