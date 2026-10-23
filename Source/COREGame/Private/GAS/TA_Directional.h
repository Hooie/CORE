// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "TA_Directional.generated.h"

/**
 * 
 */
UCLASS()
class ATA_Directional : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()
	
public:
	ATA_Directional();

	void SetDashDistance(float InDashDistance);
	void SetShouldDrawDebug(bool bInShouldDrawDebug) { bShouldDrawDebug = bInShouldDrawDebug; }

	virtual void StartTargeting(UGameplayAbility* Ability) override;
	virtual void Tick(float DeltaTime) override;
	virtual void ConfirmTargetingAndContinue() override;
	virtual void CancelTargeting() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Component")
	class UDecalComponent* DecalComp;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	float YawCorrectionOffset = 90.f;

	float DashDistance = 600.f;
	bool bShouldDrawDebug = false;

	FVector CachedAimDirection = FVector::ForwardVector;

	FVector GetAimGroundLocation() const;
	
	FVector GetTargetPoint() const;

	FVector GetAvatarFeetLocation() const;
};
