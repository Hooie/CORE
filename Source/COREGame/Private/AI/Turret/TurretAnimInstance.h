// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "TurretAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class UTurretAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite, Category = "Turret")
	bool OpenPanel = false;

	UPROPERTY(BlueprintReadWrite, Category = "Turret")
	float DistanceToTarget = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "Turret")
	int32 Tower_Variation = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Turret")
	float TurretDamage = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "Turret")
	bool Aiming = false;

	UPROPERTY(BlueprintReadWrite, Category = "Turret")
	bool OpenShield = false;

	UPROPERTY(BlueprintReadWrite, Category = "Turret")
	bool IsHit = false;

	UPROPERTY(BlueprintReadWrite, Category = "Turret")
	int32 HitVariation = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Turret")
	bool isFiring = false;

	UPROPERTY(BlueprintReadWrite, Category = "Turret")
	FVector AimLookAtLocation = FVector(100.f, 0.f, 250.f);
};
