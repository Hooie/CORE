// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Murdock/GCN_MurdockBeamEffect.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"

AGCN_MurdockBeamEffect::AGCN_MurdockBeamEffect()
{
	PrimaryActorTick.bCanEverTick = false;
	bAutoDestroyOnRemove = true;
}

// ✅ 루프용 — 레이저 사이트
bool AGCN_MurdockBeamEffect::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	SpawnBeamFromContext(Parameters);
	return true;
}

bool AGCN_MurdockBeamEffect::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if ( IsValid(BeamComponent) )
	{
		BeamComponent->Deactivate();
		BeamComponent = nullptr;
	}
	return true;
}

// ✅ 단발용 — 블래스트 빔
bool AGCN_MurdockBeamEffect::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	SpawnBeamFromContext(Parameters);

	GetWorld()->GetTimerManager().SetTimer(OneShotTimerHandle, this, &AGCN_MurdockBeamEffect::FinishOneShot, OneShotLifeDuration, false);

	return true;
}

void AGCN_MurdockBeamEffect::SpawnBeamFromContext(const FGameplayCueParameters& Parameters)
{
	if ( !BeamVFX )
		return;

	const FHitResult* HitData = Parameters.EffectContext.GetHitResult();
	if ( !HitData )
		return;

	FVector StartLocation = HitData->TraceStart;
	FVector EndLocation = HitData->Location;

	BeamComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamVFX, StartLocation, FRotator::ZeroRotator, true, EPSCPoolMethod::None, true);
	if ( BeamComponent )
	{
		BeamComponent->SetVectorParameter(TEXT("BeamEnd"), EndLocation); // ✅ 나이아가라 이펙터 쪽에 "BeamEnd" 벡터 파라미터로 끝점 전달
		BeamComponent->SetWorldLocation(StartLocation);

		FVector Direction = (EndLocation - StartLocation).GetSafeNormal();
		BeamComponent->SetWorldRotation(Direction.Rotation());
	}
}

void AGCN_MurdockBeamEffect::FinishOneShot()
{
	if ( IsValid(BeamComponent) )
	{
		BeamComponent->Deactivate();
		BeamComponent = nullptr;
	}

	GameplayCueFinishedCallback();
}