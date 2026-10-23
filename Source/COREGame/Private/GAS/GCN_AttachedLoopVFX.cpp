// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GCN_AttachedLoopVFX.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

AGCN_AttachedLoopVFX::AGCN_AttachedLoopVFX()
{
	bAutoDestroyOnRemove = true;
}

bool AGCN_AttachedLoopVFX::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if ( !LoopVFX || !MyTarget )
		return true;

	USkeletalMeshComponent* TargetMesh = MyTarget->FindComponentByClass<USkeletalMeshComponent>();
	if ( !TargetMesh )
		return true;

	SpawnedVFXComponent = UGameplayStatics::SpawnEmitterAttached(
		LoopVFX,
		TargetMesh,
		AttachSocketName,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		false
	);

	return true;
}

bool AGCN_AttachedLoopVFX::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if ( IsValid(SpawnedVFXComponent) )
	{
		SpawnedVFXComponent->DeactivateSystem();
		SpawnedVFXComponent = nullptr;
	}

	return true;
}
