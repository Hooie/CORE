// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GCN_AttachedNiagaraVFX.generated.h"

/**
 * 
 */
UCLASS()
class AGCN_AttachedNiagaraVFX : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()
	
public:
	AGCN_AttachedNiagaraVFX();

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	class UNiagaraSystem* LoopVFX;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	FName AttachSocketName = NAME_None;

	UPROPERTY()
	class UNiagaraComponent* SpawnedVFXComponent;
};
