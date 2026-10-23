// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/TA_Directional.h"
#include "Abilities/GameplayAbility.h"
#include "Components/CapsuleComponent.h" 
#include "Components/DecalComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

ATA_Directional::ATA_Directional()
{
	PrimaryActorTick.bCanEverTick = true;

	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root Comp"));

	DecalComp = CreateDefaultSubobject<UDecalComponent>("Decal Comp");

	DecalComp->SetupAttachment(GetRootComponent());
}

void ATA_Directional::SetDashDistance(float InDashDistance)
{
	DashDistance = InDashDistance;
	DecalComp->DecalSize = FVector(300.f, DashDistance * 0.6f, 60.f);
	DecalComp->SetRelativeLocation(FVector(0.f, -(DashDistance * 0.6f), 0.f));
}

void ATA_Directional::StartTargeting(UGameplayAbility* Ability)
{
	Super::StartTargeting(Ability);

	SetActorLocation(GetAvatarFeetLocation());
}

void ATA_Directional::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if ( !PrimaryPC || !PrimaryPC->IsLocalPlayerController() )
		return;

	SetActorLocation(GetAvatarFeetLocation());

	FVector CamLoc;
	FRotator CamRot;
	PrimaryPC->GetPlayerViewPoint(CamLoc, CamRot);

	FVector DirToAim = CamRot.Vector();
	DirToAim.Z = 0.f;
	DirToAim = DirToAim.GetSafeNormal();

	if ( !DirToAim.IsNearlyZero() )
	{
		CachedAimDirection = DirToAim;

		float CorrectedYaw = DirToAim.Rotation().Yaw + YawCorrectionOffset;
		SetActorRotation(FRotator(0.f, CorrectedYaw, 0.f));
	}

	if ( bShouldDrawDebug )
	{
		DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + DirToAim * DashDistance, FColor::Cyan, false, -1.f, 0, 5.f);
	}
}

void ATA_Directional::ConfirmTargetingAndContinue()
{
	FVector TargetPoint = GetActorLocation() + CachedAimDirection * DashDistance;

	FGameplayAbilityTargetDataHandle TargetDataHandle;
	FGameplayAbilityTargetData_SingleTargetHit* DirectionData = new FGameplayAbilityTargetData_SingleTargetHit;
	DirectionData->HitResult.ImpactPoint = TargetPoint;
	DirectionData->HitResult.Location = TargetPoint;
	TargetDataHandle.Add(DirectionData);

	TargetDataReadyDelegate.Broadcast(TargetDataHandle);
}

void ATA_Directional::CancelTargeting()
{
	Super::CancelTargeting();
}

FVector ATA_Directional::GetAimGroundLocation() const
{
	if ( !PrimaryPC )
		return GetActorLocation() + GetActorForwardVector() * DashDistance;

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

FVector ATA_Directional::GetAvatarFeetLocation() const
{
	if ( OwningAbility )
	{
		if ( AActor* AvatarActor = OwningAbility->GetCurrentActorInfo()->AvatarActor.Get() )
		{
			if ( ACharacter* AvatarCharacter = Cast<ACharacter>(AvatarActor) )
			{
				if ( UCapsuleComponent* Capsule = AvatarCharacter->GetCapsuleComponent() )
				{
					FVector Loc = AvatarActor->GetActorLocation();
					Loc.Z -= Capsule->GetScaledCapsuleHalfHeight();
					return Loc;
				}
			}
			return AvatarActor->GetActorLocation();
		}
	}
	return GetActorLocation();
}