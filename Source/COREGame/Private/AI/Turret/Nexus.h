// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/Turret/Turret.h"
#include "Nexus.generated.h"

class ANexus;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnNexusDestroyedDelegate, ANexus* /*DestroyedNexus*/);

/**
 * 
 */
UCLASS()
class ANexus : public ATurret
{
	GENERATED_BODY()
	
public:
	ANexus();

	FOnNexusDestroyedDelegate OnNexusDestroyed;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void StartExplosion();

	float GetCameraBlendDuration() const { return CameraBlendDuration; }
	float GetResultWidgetDelay() const { return ResultWidgetDelay; }

protected:
	virtual void DestroyTower() override;
	virtual void PlayDestroyedPresentation() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Component")
	class UCameraComponent* ViewCam;

	UPROPERTY(EditDefaultsOnly, Category = "Match End", meta = (ClampMin = "0.0"))
	float CameraBlendDuration = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Match End", meta = (ClampMin = "0.0"))
	float ResultWidgetDelay = 3.f;

	UPROPERTY(ReplicatedUsing = OnRep_ExplosionStarted)
	bool bExplosionStarted = false;

	UFUNCTION()
	void OnRep_ExplosionStarted();
};
