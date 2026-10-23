// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GCN_TracerEffect.generated.h"

/**
 * 
 */
UCLASS()
class AGCN_TracerEffect : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()
	
public:
	AGCN_TracerEffect();

	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;

private:
	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* TracerMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Tracer")
	FName MuzzleSocketName = "Muzzle_01";

	UPROPERTY(EditDefaultsOnly, Category = "Tracer")
	float TracerRadius = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Tracer")
	float SourceCylinderLength = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Tracer")
	float LifeDuration = 0.15f; 

	FTimerHandle FadeTimerHandle;
	void FinishTracer();

	UPROPERTY(EditDefaultsOnly, Category = "Tracer|Debug")
	bool bDrawDebugTracer = false;
};
