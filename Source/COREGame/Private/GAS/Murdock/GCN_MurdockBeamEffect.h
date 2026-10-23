// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GCN_MurdockBeamEffect.generated.h"

/**
 * 
 */
UCLASS()
class AGCN_MurdockBeamEffect : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()
	
public:
	AGCN_MurdockBeamEffect();

	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

private:
	UPROPERTY(VisibleAnywhere)
	class UParticleSystemComponent* BeamComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	class UParticleSystem* BeamVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	bool bIsOneShot = false; // 블래스트 빔용 블루프린트에서만 true로 설정

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	float OneShotLifeDuration = 0.3f;

	FTimerHandle OneShotTimerHandle;

	void SpawnBeamFromContext(const FGameplayCueParameters& Parameters);
	void FinishOneShot();
	
	
};
