// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Muriel/TA_ConsecratedGroundPreview.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "COREGame/COREGame.h"

ATA_ConsecratedGroundPreview::ATA_ConsecratedGroundPreview()
{
	PrimaryActorTick.bCanEverTick = true;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root Comp")); 

	PreviewSphereMesh = CreateDefaultSubobject<UStaticMeshComponent>("Preview Sphere Mesh");
	PreviewSphereMesh->SetupAttachment(GetRootComponent());
	PreviewSphereMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewSphereMesh->SetCastShadow(false);

	DecalComp = CreateDefaultSubobject<UDecalComponent>("Decal Comp");
	DecalComp->SetupAttachment(GetRootComponent());
}

void ATA_ConsecratedGroundPreview::ConfigurePreview(float InRadius, float InMaxRange)
{
	Radius = InRadius;

	float TargetScale = Radius / 50.f;
	PreviewSphereMesh->SetWorldScale3D(FVector(TargetScale));
	DecalComp->DecalSize = FVector{ Radius };
}

void ATA_ConsecratedGroundPreview::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if ( PrimaryPC && PrimaryPC->IsLocalPlayerController() )
	{
		SetActorLocation(GetTargetPoint());
	}
}

void ATA_ConsecratedGroundPreview::ConfirmTargetingAndContinue()
{
	FGameplayAbilityTargetData_SingleTargetHit* HitLoc = new FGameplayAbilityTargetData_SingleTargetHit;
	HitLoc->HitResult.ImpactPoint = GetActorLocation();
	HitLoc->HitResult.Location = GetActorLocation();

	FGameplayAbilityTargetDataHandle TargetDataHandle;
	TargetDataHandle.Add(HitLoc);

	TargetDataReadyDelegate.Broadcast(TargetDataHandle);
}

FVector ATA_ConsecratedGroundPreview::GetTargetPoint() const
{
	if ( !PrimaryPC || !PrimaryPC->IsLocalPlayerController() )
		return GetActorLocation();

	FHitResult TraceResult;
	FVector ViewLoc;
	FRotator ViewRot;
	PrimaryPC->GetPlayerViewPoint(ViewLoc, ViewRot);

	FVector TraceEnd = ViewLoc + ViewRot.Vector() * MaxRange;

	GetWorld()->LineTraceSingleByChannel(TraceResult, ViewLoc, TraceEnd, ECC_Visibility);

	if ( !TraceResult.bBlockingHit )
	{
		GetWorld()->LineTraceSingleByChannel(TraceResult, TraceEnd, TraceEnd + FVector::DownVector * TNumericLimits<float>::Max(), ECC_Target);
	}

	if ( !TraceResult.bBlockingHit )
	{
		return GetActorLocation();
	}

	return TraceResult.ImpactPoint;
}