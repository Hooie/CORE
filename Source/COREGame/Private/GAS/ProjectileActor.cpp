// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/ProjectileActor.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AProjectileActor::AProjectileActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* RootComp = CreateDefaultSubobject<USceneComponent>("Root Comp");
	SetRootComponent(RootComp);

	bReplicates = true;
}

void AProjectileActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AProjectileActor, MoveDir);
	DOREPLIFETIME(AProjectileActor, TeamId);
	DOREPLIFETIME(AProjectileActor, ProjectileSpeed);
}

void AProjectileActor::ShootProjectile(float InSpeed, float InMaxDistance, const AActor* InTarget, FGenericTeamId InTeamId, FGameplayEffectSpecHandle InHitEffectHandle)
{
	Target = InTarget;
	ProjectileSpeed = InSpeed;

	FRotator OwnerViewRot = GetActorRotation();
	SetGenericTeamId(InTeamId);

	if ( GetOwner() && InTarget )
	{
		FVector OwnerViewLoc;
		GetOwner()->GetActorEyesViewPoint(OwnerViewLoc, OwnerViewRot);
	}

	MoveDir = OwnerViewRot.Vector();
	HitEffectSpecHandle = InHitEffectHandle;

	float TravelMaxTime = InMaxDistance / InSpeed;

	GetWorld()->GetTimerManager().SetTimer(ShootTimerHandle, this, &AProjectileActor::TravelMaxDistanceReached, TravelMaxTime);
}

void AProjectileActor::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
	TeamId = NewTeamId;
}

void AProjectileActor::NotifyActorBeginOverlap(AActor* OtherActor)
{
	if (!OtherActor || OtherActor == GetOwner())
		return;

	if (GetTeamAttitudeTowards(*OtherActor) != ETeamAttitude::Hostile)
		return;

	UAbilitySystemComponent* OtherASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if (IsValid(OtherASC))
	{
		if (HasAuthority() && HitEffectSpecHandle.IsValid())
		{
			OtherASC->ApplyGameplayEffectSpecToSelf(*HitEffectSpecHandle.Data.Get());
			
			GetWorld()->GetTimerManager().ClearTimer(ShootTimerHandle);
		}

		FHitResult HitResult;
		HitResult.ImpactPoint = GetActorLocation();
		HitResult.ImpactNormal = GetActorForwardVector();

		SendLocalGameplayCue(OtherActor, HitResult);

		GetWorld()->GetTimerManager().ClearTimer(ShootTimerHandle);
		Destroy();
	}
}

bool AProjectileActor::TryBlockByWorld(const FVector& StartLocation, const FVector& EndLocation)
{
	if ( !bBlockedByWorld )
		return false;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(GetOwner());

	FHitResult WorldHit;
	bool bHitWorld = GetWorld()->LineTraceSingleByChannel(WorldHit, StartLocation, EndLocation, WorldBlockChannel, Params);

	if ( !bHitWorld )
		return false;

	SetActorLocation(WorldHit.ImpactPoint);

	SendLocalGameplayCue(this, WorldHit);

	Destroy();
	return true;
}

// Called when the game starts or when spawned
void AProjectileActor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AProjectileActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority())
	{
		if (Target)
		{
			MoveDir = (Target->GetTargetLocation() - GetActorLocation()).GetSafeNormal();
		}
	}

	FVector OldLocation = GetActorLocation();
	FVector NewLocation = OldLocation + MoveDir * ProjectileSpeed * DeltaTime;

	if ( TryBlockByWorld(OldLocation, NewLocation) )
		return;

	SetActorLocation(GetActorLocation() + MoveDir * ProjectileSpeed * DeltaTime);
}

void AProjectileActor::TravelMaxDistanceReached()
{
	if( MaxDistanceDestroyVFX )
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MaxDistanceDestroyVFX, GetActorLocation(), GetActorRotation());
	}

	Destroy();
}

void AProjectileActor::SendLocalGameplayCue(AActor* CueTargetActor, const FHitResult& HitResult, bool bWorldHit)
{
	FGameplayCueParameters CueParam;
	CueParam.Location = HitResult.ImpactPoint;
	CueParam.Normal = HitResult.ImpactNormal;

	if ( bWorldHit )
	{
		UAbilitySystemGlobals::Get().GetGameplayCueManager()->HandleGameplayCue(CueTargetActor, WorldHitGameplayCueTag, EGameplayCueEvent::Executed, CueParam);
	}
	else
	{
		UAbilitySystemGlobals::Get().GetGameplayCueManager()->HandleGameplayCue(CueTargetActor, HitGameplayCueTag, EGameplayCueEvent::Executed, CueParam);
	}
}

