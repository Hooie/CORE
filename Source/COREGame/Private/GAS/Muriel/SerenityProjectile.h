// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/ProjectileActor.h"
#include "SerenityProjectile.generated.h"

/**
 * 
 */
UCLASS()
class ASerenityProjectile : public AProjectileActor
{
	GENERATED_BODY()
	
	
public:
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	
};
