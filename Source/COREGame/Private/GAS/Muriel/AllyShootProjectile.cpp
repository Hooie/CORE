// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Muriel/AllyShootProjectile.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/SphereComponent.h"
#include "GameplayCueManager.h"
#include "GAS/CAbilitySystemStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Net/UnrealNetwork.h"


AAllyShootProjectile::AAllyShootProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* RootComp = CreateDefaultSubobject<USceneComponent>("Root Comp");
	SetRootComponent(RootComp);

	CollisionComponent = CreateDefaultSubobject<USphereComponent>("Collision Component");
	CollisionComponent->InitSphereRadius(CollisionRadius);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly); 
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); 
	CollisionComponent->SetGenerateOverlapEvents(true);
	CollisionComponent->SetupAttachment(GetRootComponent());

	ProjectileVFX = CreateDefaultSubobject<UParticleSystemComponent>("Projectile VFX");
	ProjectileVFX->SetupAttachment(GetRootComponent());

	bReplicates = true;
}

void AAllyShootProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAllyShootProjectile, TeamId);
	DOREPLIFETIME(AAllyShootProjectile, MoveDir);
	DOREPLIFETIME(AAllyShootProjectile, ProjectileSpeed);
}

void AAllyShootProjectile::ShootProjectile(float InSpeed, float InMaxDistance, const AActor* InTarget, FGenericTeamId InTeamId, FGameplayEffectSpecHandle InHitEffectHandle)
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

	GetWorld()->GetTimerManager().SetTimer(ShootTimerHandle, this, &AAllyShootProjectile::TravelMaxDistanceReached, TravelMaxTime);
}

void AAllyShootProjectile::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
	TeamId = NewTeamId;
}

void AAllyShootProjectile::NotifyActorBeginOverlap(AActor* OtherActor)
{
	if ( !OtherActor || OtherActor == GetOwner() )
		return;

	if ( GetTeamAttitudeTowards(*OtherActor) == ETeamAttitude::Hostile )
		return;

	if ( !UCAbilitySystemStatics::IsHero(OtherActor) )
		return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if ( IsValid(TargetASC) )
	{
		if ( HasAuthority() )
		{
			if ( HitEffectSpecHandle.IsValid() )
			{
				TargetASC->ApplyGameplayEffectSpecToSelf(*HitEffectSpecHandle.Data.Get());
			}

			OnAllyHit.Broadcast(OtherActor, TargetASC);

			GetWorld()->GetTimerManager().ClearTimer(ShootTimerHandle);
		}

		FHitResult HitResult;
		HitResult.ImpactPoint = GetActorLocation();
		HitResult.ImpactNormal = GetActorForwardVector();

		SendLocalGameplayCue(OtherActor, HitResult);

		Destroy();
	}
}

// Called when the game starts or when spawned
void AAllyShootProjectile::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AAllyShootProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if ( HasAuthority() )
	{
		if ( Target )
		{
			MoveDir = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
		}
	}
	SetActorLocation(GetActorLocation() + MoveDir * ProjectileSpeed * DeltaTime);
}

void AAllyShootProjectile::TravelMaxDistanceReached()
{
	Destroy();
}

void AAllyShootProjectile::SendLocalGameplayCue(AActor* CueTargetActor, const FHitResult& HitResult)
{
	FGameplayCueParameters CueParam;
	CueParam.Location = HitResult.ImpactPoint;
	CueParam.Normal = HitResult.ImpactNormal;

	UAbilitySystemGlobals::Get().GetGameplayCueManager()->HandleGameplayCue(CueTargetActor, HitGameplayCueTag, EGameplayCueEvent::Executed, CueParam);
}