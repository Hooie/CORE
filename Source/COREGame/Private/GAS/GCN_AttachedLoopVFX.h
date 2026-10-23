// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GCN_AttachedLoopVFX.generated.h"

/**
 * 
 */
UCLASS()
class AGCN_AttachedLoopVFX : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()
	
public:
	AGCN_AttachedLoopVFX();

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	class UParticleSystem* LoopVFX;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	FName AttachSocketName = NAME_None;

	UPROPERTY()
	class UParticleSystemComponent* SpawnedVFXComponent;
};
