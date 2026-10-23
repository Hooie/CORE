// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/CPlayerCharacter.h"
#include "CMurdockCharacter.generated.h"

/**
 * 
 */
UCLASS()
class ACMurdockCharacter : public ACPlayerCharacter
{
	GENERATED_BODY()
	
public:
	ACMurdockCharacter();
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Ammo")
	class UAmmoComponent* AmmoComponent;

private:
	
	
};
