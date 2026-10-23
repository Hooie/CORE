// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/CMurdockCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Player/AmmoComponent.h"

ACMurdockCharacter::ACMurdockCharacter()
{
	AmmoComponent = CreateDefaultSubobject<UAmmoComponent>("Ammo Component");
}

void ACMurdockCharacter::BeginPlay()
{
	Super::BeginPlay();

	if ( HasAuthority() )
	{
		GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	}
}
