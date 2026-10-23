// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "TA_ConsecratedGroundPreview.generated.h"

/**
 * 
 */
UCLASS()
class ATA_ConsecratedGroundPreview : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()
	
public:
	ATA_ConsecratedGroundPreview();

	void ConfigurePreview(float InRadius, float InMaxRange);

	virtual void Tick(float DeltaTime) override;

	virtual void ConfirmTargetingAndContinue() override;
	
private:
	UPROPERTY(VisibleAnywhere, Category = "Visual")
	class UDecalComponent* DecalComp;

	UPROPERTY(VisibleAnywhere)
	class UStaticMeshComponent* PreviewSphereMesh;

	float Radius = 500.f;
	float MaxRange = 2000.f;

	FVector GetTargetPoint() const;
};
