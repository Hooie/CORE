// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "TA_Shotgun.generated.h"

/**
 * 
 */
UCLASS()
class ATA_Shotgun : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()
	
public:
	ATA_Shotgun();

	void ConfigureShotgun(float InSpreadAngle, int32 InShotCount, float InRange, bool bInShouldDrawDebug);

	virtual void StartTargeting(UGameplayAbility* Ability) override;
	virtual void Tick(float DeltaTime) override;
	virtual void ConfirmTargetingAndContinue() override;
	virtual void CancelTargeting() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Component")
	class USceneComponent* RootComp;
	
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	class UMaterialInstance* ArrowDecalMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	float ArrowDecalWidth = 60.f;

	TArray<class UDecalComponent*> DecalComponents;

	float SpreadAngle = 100.f;
	int32 ShotCount = 6;
	float Range = 1000.f;
	bool bShouldDrawDebug = false;

	FVector GetAimGroundLocation() const;
};
