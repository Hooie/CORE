// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Kwang/TA_Sword.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystemComponent.h"

ATA_Sword::ATA_Sword()
{
	RootComp = CreateDefaultSubobject<USceneComponent>("Root Comp");
	SetRootComponent(RootComp);

	DetectionSphereComponent = CreateDefaultSubobject<USphereComponent>("Detection Sphere Component");
	DetectionSphereComponent->SetupAttachment(GetRootComponent());
	DetectionSphereComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	DetectionSphereComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	DetectionSphereComponent->OnComponentBeginOverlap.AddDynamic(this, &ATA_Sword::ActorInSwordRange);
	DetectionSphereComponent->OnComponentEndOverlap.AddDynamic(this, &ATA_Sword::ActorLeftSwordRange);

	GroundDecalComponent = CreateDefaultSubobject<UDecalComponent>("Ground Decal Component");
	GroundDecalComponent->SetupAttachment(GetRootComponent());

	SwordMeshComp = CreateDefaultSubobject<UStaticMeshComponent>("Sword Mesh Comp");
	SwordMeshComp->SetupAttachment(GetRootComponent());

	bReplicates = true;
	ShouldProduceTargetDataOnServer = true;
	PrimaryActorTick.bCanEverTick = false;

	PlasmaVFX = CreateDefaultSubobject<UParticleSystemComponent>("Plasma VFX");
	PlasmaVFX->SetupAttachment(GetRootComponent());

	EffectVFX = CreateDefaultSubobject<UParticleSystemComponent>("Effect VFX");
	EffectVFX->SetupAttachment(GetRootComponent());

	ImpactVFX = CreateDefaultSubobject<UParticleSystemComponent>("Impact VFX");
	ImpactVFX->SetupAttachment(GetRootComponent());

	LandingVFX = CreateDefaultSubobject<UParticleSystemComponent>("Landing VFX");
	LandingVFX->SetupAttachment(GetRootComponent());
}

void ATA_Sword::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATA_Sword, TeamId);
	DOREPLIFETIME_CONDITION_NOTIFY(ATA_Sword, SwordRange, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME(ATA_Sword, ReplicatedLocation);
}

void ATA_Sword::ConfigureSword(float InSwordRange, float InSwordBindInterval, float InSwordDuration, const FGenericTeamId& InTeamId)
{
	SwordBindInterval = InSwordBindInterval;
	DetectionSphereComponent->SetSphereRadius(InSwordRange);
	SetGenericTeamId(InTeamId);
	SwordDurtaion = InSwordDuration;
	SwordRange = InSwordRange;
}

void ATA_Sword::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	TeamId = NewTeamID;
}

void ATA_Sword::StartTargeting(UGameplayAbility* Ability)
{

	UE_LOG(LogTemp, Warning, TEXT("TA_Sword: StartTargeting - HasAuthority: %s"),
		HasAuthority() ? TEXT("true") : TEXT("false"));

	Super::StartTargeting(Ability);

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(SwordDurationTimerHandle, this, &ATA_Sword::StopSword, SwordDurtaion);
		World->GetTimerManager().SetTimer(FuryDisableTimerHandle, this, &ATA_Sword::DisableFury, SwordDurtaion - 3.f);
	}
}

void ATA_Sword::ConfirmTargetingAndContinue()
{
	StopSword();
}

void ATA_Sword::CancelTargeting()
{
	StopSword();
	Super::CancelTargeting();
}

void ATA_Sword::BeginBindTimer()
{
	UE_LOG(LogTemp, Warning, TEXT("TA_Sword: BeginBindTimer called - HasAuthority: %s"),
		HasAuthority() ? TEXT("true") : TEXT("false"));

	BindTargets();

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(SwordBindIntervalTimerHandle, this, &ATA_Sword::BindTargets, SwordBindInterval, true);
	}
}

void ATA_Sword::SetSwordLocation(const FVector& NewLocation)
{
	ReplicatedLocation = NewLocation;
	SetActorLocation(ReplicatedLocation);
}

void ATA_Sword::EarlyTerminate()
{
	if (SwordDurationTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(SwordDurationTimerHandle);
	}

	StopSword();
}

void ATA_Sword::ForceStop()
{
	GetWorld()->GetTimerManager().ClearTimer(SwordDurationTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(SwordBindIntervalTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(FuryDisableTimerHandle);
	Destroy();
}

#if WITH_EDITOR
void ATA_Sword::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if ( PropertyName == GET_MEMBER_NAME_CHECKED(ATA_Sword, SwordRange) )
	{
		DetectionSphereComponent->SetSphereRadius(SwordRange);
		FVector DecalSize = GroundDecalComponent->DecalSize;
		GroundDecalComponent->DecalSize = FVector{ DecalSize.X, SwordRange, SwordRange };
	}
}
#endif

void ATA_Sword::DisableFury()
{
	OnFuryDisabled.Broadcast();
}

void ATA_Sword::OnRep_SwordRange()
{
	DetectionSphereComponent->SetSphereRadius(SwordRange);
}

void ATA_Sword::OnRep_SwordLocation()
{
	SetActorLocation(ReplicatedLocation);
}

void ATA_Sword::ActorInSwordRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TryAddTarget(OtherActor);
}

void ATA_Sword::ActorLeftSwordRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	RemoveTarget(OtherActor);
}

void ATA_Sword::TryAddTarget(AActor* OtherTarget)
{
	if (!OtherTarget || ActorsInRange.Contains(OtherTarget))
		return;

	if (GetTeamAttitudeTowards(*OtherTarget) != ETeamAttitude::Hostile)
		return;

	ActorsInRange.Add(OtherTarget);
}

void ATA_Sword::RemoveTarget(AActor* OtherTarget)
{
	if (!OtherTarget)
		return;

	if (ActorsInRange.Contains(OtherTarget))
		ActorsInRange.Remove(OtherTarget);
}

void ATA_Sword::StopSword()
{
	UWorld* World = GetWorld();
	if (World)
	{
		if (SwordBindIntervalTimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(SwordBindIntervalTimerHandle);
		}
	}

	TArray<TWeakObjectPtr<AActor>> FinalTargets;

	for (AActor* Target : ActorsInRange)
	{
		FinalTargets.Add(Target);
	}

	FGameplayAbilityTargetDataHandle TargetDataHandle;
	FGameplayAbilityTargetData_ActorArray* TargetActorArray = new FGameplayAbilityTargetData_ActorArray;
	TargetActorArray->SetActors(FinalTargets);
	TargetDataHandle.Add(TargetActorArray);

	TargetDataReadyDelegate.Broadcast(TargetDataHandle);

}

void ATA_Sword::BindTargets()
{
	TArray<AActor*> OverlappingActors;
	DetectionSphereComponent->GetOverlappingActors(OverlappingActors);
	for (AActor* Actor : OverlappingActors)
	{
		TryAddTarget(Actor);
	}

	for (AActor* Target : ActorsInRange)
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);

		if (TargetASC)
		{
			TargetASC->BP_ApplyGameplayEffectToSelf(BindandDisarmEffect, 1, FGameplayEffectContextHandle());
		}
	}
}