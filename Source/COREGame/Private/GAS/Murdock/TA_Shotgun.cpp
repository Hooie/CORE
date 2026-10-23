// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Murdock/TA_Shotgun.h"
#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "Components/DecalComponent.h"

ATA_Shotgun::ATA_Shotgun()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComp = CreateDefaultSubobject<USceneComponent>("Root Comp");
	SetRootComponent(RootComp);
}

void ATA_Shotgun::ConfigureShotgun(float InSpreadAngle, int32 InShotCount, float InRange, bool bInShouldDrawDebug)
{
	SpreadAngle = InSpreadAngle;
	ShotCount = InShotCount;
	Range = InRange;
	bShouldDrawDebug = bInShouldDrawDebug;

	float HalfAngle = SpreadAngle * 0.5f;
	float AngleStep = (ShotCount > 1) ? (SpreadAngle / (ShotCount - 1)) : 0.f;

	for ( int32 i = 0; i < ShotCount; ++i )
	{
		float ShotYawOffset = (ShotCount > 1) ? (-HalfAngle + AngleStep * i) : 0.f;

		UDecalComponent* NewDecal = NewObject<UDecalComponent>(this);
		NewDecal->SetupAttachment(GetRootComponent());

		NewDecal->SetDecalMaterial(ArrowDecalMaterial);
		NewDecal->DecalSize = FVector(100.f, ArrowDecalWidth, Range * 0.5f);
		NewDecal->SetRelativeRotation(FRotator(90.f, 0.f, 0.f));
		NewDecal->SetRelativeLocation(FVector(0.f, -(Range * 0.5f), 0.f));
		NewDecal->AddLocalRotation(FRotator(0.f, 0.f, ShotYawOffset));

		NewDecal->RegisterComponent();

		DecalComponents.Add(NewDecal);
	}
}

void ATA_Shotgun::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);
}

void ATA_Shotgun::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if ( !PrimaryPC || !PrimaryPC->IsLocalPlayerController() )
		return;

	if ( OwningAbility )
	{
		if ( AActor* AvatarActor = OwningAbility->GetCurrentActorInfo()->AvatarActor.Get() )
		{
			SetActorLocation(AvatarActor->GetActorLocation());
		}
	}

	FVector AimGroundLocation = GetAimGroundLocation();
	FVector DirToAim = (AimGroundLocation - GetActorLocation());
	DirToAim.Z = 0.f;
	DirToAim = DirToAim.GetSafeNormal();

	if ( !DirToAim.IsNearlyZero() )
	{
		SetActorRotation(FRotator(0.f, DirToAim.Rotation().Yaw, 0.f)); 
	}

	if ( bShouldDrawDebug )
	{
		float HalfAngle = SpreadAngle * 0.5f;
		float AngleStep = (ShotCount > 1) ? (SpreadAngle / (ShotCount - 1)) : 0.f;
		for ( int32 i = 0; i < ShotCount; ++i )
		{
			float PelletYawOffset = (ShotCount > 1) ? (-HalfAngle + AngleStep * i) : 0.f;
			FVector PelletDir = FRotator(0.f, GetActorRotation().Yaw + PelletYawOffset, 0.f).Vector();
			DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + PelletDir * Range, FColor::Cyan, false, -1.f, 0, 3.f);
		}
	}
}

void ATA_Shotgun::ConfirmTargetingAndContinue()
{
	FVector TargetPoint = GetActorLocation() + GetActorForwardVector() * Range;

	FGameplayAbilityTargetDataHandle TargetDataHandle;
	FGameplayAbilityTargetData_SingleTargetHit* DirectionData = new FGameplayAbilityTargetData_SingleTargetHit;
	DirectionData->HitResult.ImpactPoint = TargetPoint;
	TargetDataHandle.Add(DirectionData);

	TargetDataReadyDelegate.Broadcast(TargetDataHandle);
}

void ATA_Shotgun::CancelTargeting()
{
	Super::CancelTargeting();
}

FVector ATA_Shotgun::GetAimGroundLocation() const
{
	if ( !PrimaryPC )
		return GetActorLocation() + GetActorForwardVector() * Range;

	FHitResult HitResult;
	FVector CamLoc;
	FRotator CamRot;
	PrimaryPC->GetPlayerViewPoint(CamLoc, CamRot);
	FVector TraceEnd = CamLoc + CamRot.Vector() * 10000.f;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if ( GetWorld()->LineTraceSingleByChannel(HitResult, CamLoc, TraceEnd, ECC_Visibility, Params) )
	{
		return HitResult.Location;
	}

	return TraceEnd;
}