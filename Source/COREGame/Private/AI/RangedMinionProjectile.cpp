// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/RangedMinionProjectile.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameplayCueManager.h"
#include "GAS/CAbilitySystemStatics.h"
#include "Net/UnrealNetwork.h"
#include "Framework/CoreBench.h"

const FRotator ARangedMinionProjectile::MeshOrientationOffset = FRotator(90.f, 0.f, 0.f);

ARangedMinionProjectile::ARangedMinionProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(false);
	SetNetUpdateFrequency(5.f);
	SetMinNetUpdateFrequency(2.f);

	CollisionComponent = CreateDefaultSubobject<UCapsuleComponent>("Collision Component");
	SetRootComponent(CollisionComponent);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComponent->SetGenerateOverlapEvents(true);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>("Projectile Mesh");
	ProjectileMesh->SetupAttachment(GetRootComponent());
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileVFX = CreateDefaultSubobject<UParticleSystemComponent>("Projectile VFX");
	ProjectileMesh->SetupAttachment(GetRootComponent());
}

void ARangedMinionProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ARangedMinionProjectile, MoveDir, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ARangedMinionProjectile, ProjectileSpeed, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(ARangedMinionProjectile, DesignatedTarget, COND_InitialOnly);
}

void ARangedMinionProjectile::Launch(AActor* InDesignatedTarget, float InSpeed, float InMaxDistance, FGameplayEffectSpecHandle InHitEffectHandle)
{
	DesignatedTarget = InDesignatedTarget;
	ProjectileSpeed = InSpeed;
	
	MoveDir = GetActorForwardVector();
	HitEffectSpecHandle = InHitEffectHandle;

	FQuat FacingQuat = MoveDir.Rotation().Quaternion();
	FQuat OffsetQuat = MeshOrientationOffset.Quaternion();
	SetActorRotation(FacingQuat * OffsetQuat);

	if ( HasAuthority() )
	{
		float TravelMaxTime = InMaxDistance / InSpeed;
		GetWorldTimerManager().SetTimer(LifetimeTimerHandle, this, &ARangedMinionProjectile::ExpireProjectile, TravelMaxTime, false);
	}
}

void ARangedMinionProjectile::NotifyActorBeginOverlap(AActor* OtherActor)
{
	if ( !OtherActor )
		return;

	if ( OtherActor != DesignatedTarget.Get() )
	{
		return;
	}

	FHitResult HitResult;
	HitResult.ImpactPoint = GetActorLocation();
	HitResult.ImpactNormal = GetActorForwardVector();

	SendLocalGameplayCue(OtherActor, HitResult);

	UAbilitySystemComponent* OtherASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if ( IsValid(OtherASC) )
	{
		if ( HasAuthority() && HitEffectSpecHandle.IsValid() ) 
		{
			OtherASC->ApplyGameplayEffectSpecToSelf(*HitEffectSpecHandle.Data.Get());
			GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);
		}
	}

	if ( HasAuthority() )
	{
		Destroy();
	}
	else
	{
		SetActorHiddenInGame(true);
		SetActorTickEnabled(false);
	}
}

// Called when the game starts or when spawned
void ARangedMinionProjectile::BeginPlay()
{
	Super::BeginPlay();

	static const bool bNoOptProjectile = CoreBench::IsOptOff(TEXT("NoOptProjectile"));
	if ( HasAuthority() && bNoOptProjectile )
	{
		SetReplicateMovement(true);
		SetNetUpdateFrequency(100.f);
	}
}

// Called every frame
void ARangedMinionProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if ( DesignatedTarget.IsValid() && !UCAbilitySystemStatics::IsActorDead(DesignatedTarget.Get()) )
	{
		MoveDir = (DesignatedTarget->GetTargetLocation() - GetActorLocation()).GetSafeNormal();
	}
	
	SetActorLocation(GetActorLocation() + MoveDir * ProjectileSpeed * DeltaTime);

	FQuat FacingQuat = MoveDir.Rotation().Quaternion();
	FQuat OffsetQuat = MeshOrientationOffset.Quaternion();
	SetActorRotation(FacingQuat * OffsetQuat);
}

void ARangedMinionProjectile::ExpireProjectile()
{
	Destroy();
}

void ARangedMinionProjectile::SendLocalGameplayCue(AActor* CueTargetActor, const FHitResult& HitResult)
{
	FGameplayCueParameters CueParam;
	CueParam.Location = HitResult.ImpactPoint;
	CueParam.Normal = HitResult.ImpactNormal;

	UAbilitySystemGlobals::Get().GetGameplayCueManager()->HandleGameplayCue(CueTargetActor, HitGameplayCueTag, EGameplayCueEvent::Executed, CueParam);
}