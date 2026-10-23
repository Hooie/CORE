// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GCN_LocationParticle.h"
#include "Kismet/GameplayStatics.h"

bool UGCN_LocationParticle::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if ( ParticleVFX )
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ParticleVFX, Parameters.Location, FRotator::ZeroRotator);
	}

	return true;
}