// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GCN_AttachedNiagaraVFX.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

AGCN_AttachedNiagaraVFX::AGCN_AttachedNiagaraVFX()
{
	bAutoDestroyOnRemove = true;
}

bool AGCN_AttachedNiagaraVFX::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if ( !LoopVFX || !MyTarget )
		return true;

	ACharacter* TargetCharacter = Cast<ACharacter>(MyTarget);
	USkeletalMeshComponent* TargetMesh = TargetCharacter ? TargetCharacter->GetMesh() : MyTarget->FindComponentByClass<USkeletalMeshComponent>();

	if ( !TargetMesh )
		return true;

	SpawnedVFXComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
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

bool AGCN_AttachedNiagaraVFX::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if ( IsValid(SpawnedVFXComponent) )
	{
		SpawnedVFXComponent->Deactivate();
		SpawnedVFXComponent = nullptr;
	}

	return true;
}
