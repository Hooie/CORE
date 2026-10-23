// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GCN_ShieldLoopVFX.generated.h"

/**
 * 
 */
UCLASS()
class AGCN_ShieldLoopVFX : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()
	
public:
	AGCN_ShieldLoopVFX();

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	class UParticleSystem* ShieldLoopVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FName AttachSocketName = NAME_None;

	UPROPERTY()
	class UParticleSystemComponent* SpawnedComponent;
};
