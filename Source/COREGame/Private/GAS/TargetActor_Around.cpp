// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/TargetActor_Around.h"
#include "Abilities/GameplayAbility.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "Net/UnrealNetwork.h"

ATargetActor_Around::ATargetActor_Around()
{
	RootComp = CreateDefaultSubobject<USceneComponent>("Root Comp");
	SetRootComponent(RootComp);

	DetectionSphere = CreateDefaultSubobject<USphereComponent>("Detection Sphere");
	DetectionSphere->SetupAttachment(GetRootComponent());
	DetectionSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	DetectionSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECR_Overlap);
	DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &ATargetActor_Around::ActorInDectionRange);

	bReplicates = true;
	ShouldProduceTargetDataOnServer = true;
}

void ATargetActor_Around::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATargetActor_Around, TeamId);
	DOREPLIFETIME(ATargetActor_Around, TargetDetectionRadius);
	DOREPLIFETIME(ATargetActor_Around, LocalGameplayCueTag);
}

void ATargetActor_Around::SetGenericTeamId(const FGenericTeamId& InTeamID)
{
	TeamId = InTeamID;
}

void ATargetActor_Around::ConfigureDetection(float InDetectionRadius, const FGenericTeamId& InTeamId, const FGameplayTag& InLocalGameplayCueTag)
{
	TargetDetectionRadius = InDetectionRadius;
	DetectionSphere->SetSphereRadius(TargetDetectionRadius);
	SetGenericTeamId(InTeamId);
	LocalGameplayCueTag = InLocalGameplayCueTag;
}

void ATargetActor_Around::OnRep_TargetDetectionRadiusReplicated()
{
	DetectionSphere->SetSphereRadius(TargetDetectionRadius);
}

void ATargetActor_Around::ActorInDectionRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor)
		return;

	AActor* AvatarActor = nullptr;
	if (OwningAbility)
	{
		AvatarActor = OwningAbility->GetAvatarActorFromActorInfo();
	}

	if (OtherActor == AvatarActor || OtherActor == this)
		return;

	if (GetTeamAttitudeTowards(*OtherActor) != ETeamAttitude::Hostile)
		return;

	if (HasAuthority())
	{
		FGameplayAbilityTargetDataHandle TargetDataHandle;
		FGameplayAbilityTargetData_ActorArray* ActorArray = new FGameplayAbilityTargetData_ActorArray;
		ActorArray->SetActors(TArray<TWeakObjectPtr<AActor>>{OtherActor});

		TargetDataHandle.Add(ActorArray);
		TargetDataReadyDelegate.Broadcast(TargetDataHandle);
	}

	FHitResult HitResult;
	HitResult.ImpactPoint = OtherActor->GetActorLocation();
	HitResult.ImpactNormal = (GetActorLocation() - OtherActor->GetActorLocation()).GetSafeNormal();
	UCAbilitySystemStatics::SendLocalGameplayCue(OtherActor, HitResult, LocalGameplayCueTag);
}
