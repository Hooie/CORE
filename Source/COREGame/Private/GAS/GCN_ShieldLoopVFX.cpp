// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GCN_ShieldLoopVFX.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"

AGCN_ShieldLoopVFX::AGCN_ShieldLoopVFX()
{
	bAutoDestroyOnRemove = true;
}

bool AGCN_ShieldLoopVFX::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if ( !ShieldLoopVFX || !MyTarget )
		return true;

	if ( ACharacter* TargetCharacter = Cast<ACharacter>(MyTarget) )
	{
		SpawnedComponent = UGameplayStatics::SpawnEmitterAttached(
			ShieldLoopVFX, TargetCharacter->GetMesh(), AttachSocketName,
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget
		);
	}

	return true;
}

bool AGCN_ShieldLoopVFX::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if ( IsValid(SpawnedComponent) )
	{
		SpawnedComponent->DeactivateSystem();
		SpawnedComponent = nullptr;
	}

	return true;
}
