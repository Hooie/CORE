// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Murdock/TA_StaticTrap.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystemComponent.h"

ATA_StaticTrap::ATA_StaticTrap()
{
	bReplicates = true;
	ShouldProduceTargetDataOnServer = true;
	PrimaryActorTick.bCanEverTick = false;

	RootComp = CreateDefaultSubobject<USceneComponent>("Root Comp");
	SetRootComponent(RootComp);

	TriggerVolume = CreateDefaultSubobject<USphereComponent>("Trigger Volume");
	TriggerVolume->SetupAttachment(GetRootComponent());
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	GroundDecalComponent = CreateDefaultSubobject<UDecalComponent>("Ground Decal Component");
	GroundDecalComponent->SetupAttachment(GetRootComponent());

	TrapMesh = CreateDefaultSubobject<USkeletalMeshComponent>("Trap Mesh");
	TrapMesh->SetupAttachment(GetRootComponent());

	SettingTrapVFX_Smoke = CreateDefaultSubobject<UParticleSystemComponent>("Setting Trap VFX Smoke");
	SettingTrapVFX_Smoke->SetupAttachment(GetRootComponent());

	SettingTrapVFX_Core = CreateDefaultSubobject<UParticleSystemComponent>("Setting Trap VFX Core");
	SettingTrapVFX_Core->SetupAttachment(GetRootComponent());
}

void ATA_StaticTrap::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATA_StaticTrap, OwnerTeamId);
	DOREPLIFETIME_CONDITION_NOTIFY(ATA_StaticTrap, TriggerRadius, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME(ATA_StaticTrap, ReplicatedLocation);
}

void ATA_StaticTrap::BeginPlay()
{
	Super::BeginPlay();

	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &ATA_StaticTrap::OnTriggerOverlap);

	if ( HasAuthority() )
	{
		GetWorldTimerManager().SetTimer(LifetimeTimerHandle, this, &ATA_StaticTrap::SelfDestructIfNotTriggered, TrapLifetime, false);
	}
}

void ATA_StaticTrap::ConfigureTrap(float InTriggerRadius, float InTrapLifetime, FGenericTeamId InTeamId, UAbilitySystemComponent* InOwnerASC, TSubclassOf<class UGameplayEffect> InDamageEffect, float InBaseDamage, float InScalingRatio, int32 InAbilityLevel, TSubclassOf<class UGameplayEffect> InBindEffect)
{
	TrapLifetime = InTrapLifetime;
	OwnerTeamId = InTeamId;
	OwnerASC = InOwnerASC;
	DamageEffect = InDamageEffect;
	BaseDamage = InBaseDamage;
	ScalingRatio = InScalingRatio;
	AbilityLevel = InAbilityLevel;
	BindEffect = InBindEffect;
	GroundDecalComponent->DecalSize = FVector{ 100.f, InTriggerRadius, InTriggerRadius };

	TriggerRadius = InTriggerRadius;
	TriggerVolume->SetSphereRadius(TriggerRadius);
}

void ATA_StaticTrap::SetTrapLocation(const FVector& Location)
{
	SetActorLocation(Location);
}

void ATA_StaticTrap::OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if ( bHasTriggered || !HasAuthority() || !OtherActor )
		return;

	IGenericTeamAgentInterface* OtherTeamInterface = Cast<IGenericTeamAgentInterface>(OtherActor);
	if ( !OtherTeamInterface || !UCAbilitySystemStatics::IsHero(OtherActor) || OtherTeamInterface->GetGenericTeamId() == OwnerTeamId )
		return;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if ( !TargetASC )
		return;

	bHasTriggered = true;
	BoundTarget = OtherActor;

	GetWorldTimerManager().ClearTimer(LifetimeTimerHandle);

	Multicast_PlayActivationVFX();

	if ( DamageEffect && OwnerASC )
	{
		FGameplayEffectSpecHandle DamageSpec = OwnerASC->MakeOutgoingSpec(DamageEffect, AbilityLevel, OwnerASC->MakeEffectContext());

		if ( DamageSpec.IsValid() )
		{
			DamageSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetAbilityBaseDamageTag(), BaseDamage);
			DamageSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetAbilityScalingRatioTag(), ScalingRatio);
			OwnerASC->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), TargetASC);
		}
	}

	if ( BindEffect )
	{
		FGameplayEffectSpecHandle BindSpec = TargetASC->MakeOutgoingSpec(BindEffect, AbilityLevel, TargetASC->MakeEffectContext());
		if ( BindSpec.IsValid() )
		{
			BoundEffectHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*BindSpec.Data.Get());

			float StartTime = 0.f;
			float AppliedDuration = 0.f;
			TargetASC->GetActiveGameplayEffects().GetGameplayEffectStartTimeAndDuration(BoundEffectHandle, StartTime, AppliedDuration);

			GetWorldTimerManager().SetTimer(BindReleaseTimerHandle, this, &ATA_StaticTrap::ReleaseBindAndDestroy, (AppliedDuration > 0.f) ? AppliedDuration : 3.f, false);
		}
		else
		{
			GetWorldTimerManager().SetTimer(BindReleaseTimerHandle, this, &ATA_StaticTrap::ReleaseBindAndDestroy, 3.f, false);
		}
	}
	else
	{
		GetWorldTimerManager().SetTimer(BindReleaseTimerHandle, this, &ATA_StaticTrap::ReleaseBindAndDestroy, 0.f, false);
	}
}

void ATA_StaticTrap::SelfDestructIfNotTriggered()
{
	if ( !bHasTriggered )
	{
		Destroy();
	}
}

void ATA_StaticTrap::ReleaseBindAndDestroy()
{
	Multicast_PlayDeactivationVFX();

	if ( AActor* Target = BoundTarget.Get() )
	{
		if ( UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target) )
		{
			TargetASC->RemoveActiveGameplayEffect(BoundEffectHandle);
		}
	}

	Destroy();
}

void ATA_StaticTrap::OnRep_TriggerRadius()
{
	TriggerVolume->SetSphereRadius(TriggerRadius);
	GroundDecalComponent->DecalSize = FVector(128.f, TriggerRadius, TriggerRadius);
}

void ATA_StaticTrap::OnRep_TrapLocation()
{
	SetActorLocation(ReplicatedLocation);
}

void ATA_StaticTrap::Multicast_PlayDeactivationVFX_Implementation()
{
	if ( IsValid(ActivationLoopVFXComponent) )
	{
		ActivationLoopVFXComponent->DeactivateSystem();
		ActivationLoopVFXComponent = nullptr;
	}

	if ( DeactivationVFX )
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DeactivationVFX, GetActorLocation(), GetActorRotation());
	}
}

void ATA_StaticTrap::Multicast_PlayActivationVFX_Implementation()
{
	if ( !ActivationLoopVFX )
		return;

	ActivationLoopVFXComponent = UGameplayStatics::SpawnEmitterAttached(
		ActivationLoopVFX,
		GetRootComponent(),
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::KeepRelativeOffset,
		false
	);
}
