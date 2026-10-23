// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/GCN_TracerEffect.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "UObject/ConstructorHelpers.h"

AGCN_TracerEffect::AGCN_TracerEffect()
{
	PrimaryActorTick.bCanEverTick = false;
	bAutoDestroyOnRemove = true;

	TracerMesh = CreateDefaultSubobject<UStaticMeshComponent>("Tracer Mesh");
	SetRootComponent(TracerMesh);

	TracerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); 
	TracerMesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if ( CylinderMeshAsset.Succeeded() )
	{
		TracerMesh->SetStaticMesh(CylinderMeshAsset.Object);
	}
}

bool AGCN_TracerEffect::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	const FHitResult* HitData = Parameters.EffectContext.GetHitResult();
	if ( !HitData )
	{
		Destroy();
		return true;
	}

	FVector StartLocation = HitData->TraceStart;
	const FVector EndLocation = HitData->Location;

	if ( const ACharacter* SourceCharacter = Cast<ACharacter>(MyTarget) )
	{
		const USkeletalMeshComponent* MeshComp = SourceCharacter->GetMesh();
		if ( MeshComp && MeshComp->DoesSocketExist(MuzzleSocketName) )
		{
			StartLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
		}
	}
	
	FVector Direction = EndLocation - StartLocation;
	float Length = Direction.Size();

	if ( Length < KINDA_SMALL_NUMBER )
	{
		Destroy();
		return true;
	}

	Direction.Normalize();

	SetActorLocation((StartLocation + EndLocation) * 0.5f);
	SetActorRotation(FRotationMatrix::MakeFromZ(Direction).Rotator());

	TracerMesh->SetWorldScale3D(FVector(TracerRadius, TracerRadius, Length / SourceCylinderLength));

	if ( bDrawDebugTracer )
	{
		DrawDebugLine(GetWorld(), StartLocation, EndLocation, FColor::Cyan, false, LifeDuration + 1.f, 0, 2.5f);
		DrawDebugSphere(GetWorld(), StartLocation, 5.f, 8, FColor::Cyan, false, LifeDuration + 1.f);
	}

	GetWorld()->GetTimerManager().SetTimer(FadeTimerHandle, this, &AGCN_TracerEffect::FinishTracer, LifeDuration, false);

	return true;
}

void AGCN_TracerEffect::FinishTracer()
{
	GameplayCueFinishedCallback(); 
}