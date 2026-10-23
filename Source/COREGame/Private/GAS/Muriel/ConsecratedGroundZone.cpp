// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Muriel/ConsecratedGroundZone.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GameFramework/Character.h"
#include "Engine/OverlapResult.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h" 

// Sets default values
AConsecratedGroundZone::AConsecratedGroundZone()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	TriggerVolume = CreateDefaultSubobject<USphereComponent>("Trigger Volume");
	SetRootComponent(TriggerVolume);
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	ZoneMesh = CreateDefaultSubobject<UStaticMeshComponent>("Zone Mesh");
	ZoneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ZoneMesh->SetCastShadow(false);
	ZoneMesh->SetupAttachment(GetRootComponent());
}

void AConsecratedGroundZone::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(AConsecratedGroundZone, Radius, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME(AConsecratedGroundZone, TeamId);
}

void AConsecratedGroundZone::ConfigureZone(float InRadius, float InDuration, FGenericTeamId InTeamId, UAbilitySystemComponent* InOwnerASC, TSubclassOf<class UGameplayEffect> InShieldEffect, TSubclassOf<class UGameplayEffect> InShieldBuffIconEffect, float InShieldDuration, TSubclassOf<class UGameplayEffect> InDamageEffect, float InBaseDamage, float InScalingRatio, int32 InAbilityLevel)
{
	Radius = InRadius;
	Duration = InDuration;
	SetGenericTeamId(InTeamId);
	OwnerASC = InOwnerASC;
	ShieldEffect = InShieldEffect;
	ShieldBuffIconEffect = InShieldBuffIconEffect;
	DamageEffect = InDamageEffect;
	BaseDamage = InBaseDamage;
	ScalingRatio = InScalingRatio;
	AbilityLevel = InAbilityLevel;
	ShieldDuration = InShieldDuration;

	TriggerVolume->SetSphereRadius(Radius);
}

void AConsecratedGroundZone::ApplyVisualScale()
{
	if ( ZoneMesh )
	{
		float TargetScale = Radius / 50.f;
		ZoneMesh->SetWorldScale3D(FVector(TargetScale));
	}
}

void AConsecratedGroundZone::DetonateEarly()
{
	if ( bDetonated )
		return;

	Detonate();
}

void AConsecratedGroundZone::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
	TeamId = NewTeamId;
}

// Called when the game starts or when spawned
void AConsecratedGroundZone::BeginPlay()
{
	Super::BeginPlay();
	
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AConsecratedGroundZone::OnOverlapBegin);

	if ( SpawnVFX )
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SpawnVFX, GetActorLocation(), FRotator::ZeroRotator);
	}

	if ( HasAuthority() )
	{
		GetWorldTimerManager().SetTimer(NaturalExpiryTimerHandle, this, &AConsecratedGroundZone::Detonate, Duration, false);
	}
}

// Called every frame
void AConsecratedGroundZone::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if ( bShouldDrawDebug )
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 24, bDetonated ? FColor::Red : FColor::Green, false, -1.f, 0, 2.f);
	}
}

void AConsecratedGroundZone::OnRep_Radius()
{
	TriggerVolume->SetSphereRadius(Radius);

	if ( ZoneMesh )
	{
		float TargetScale = Radius / 50.f;
		ZoneMesh->SetWorldScale3D(FVector(TargetScale));
	}
}

void AConsecratedGroundZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if ( !HasAuthority() || !OtherActor || bDetonated )
		return;

	IGenericTeamAgentInterface* OtherTeamInterface = Cast<IGenericTeamAgentInterface>(OtherActor);
	if ( !OtherTeamInterface || !UCAbilitySystemStatics::IsHero(OtherActor) )
		return;

	if ( OtherTeamInterface->GetGenericTeamId() != TeamId )
		return;

	if ( AlreadyShilededAllies.Contains(OtherActor) )
		return;

	UAbilitySystemComponent* OtherASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if ( !OtherASC || !ShieldEffect || !ShieldBuffIconEffect || !OwnerASC )
		return;

	AlreadyShilededAllies.Add(OtherActor);

	if ( bShouldDrawDebug )
	{
		UE_LOG(LogTemp, Warning, TEXT("[ConsecratedGround] Shield applied to %s"), *OtherActor->GetName());
		DrawDebugSphere(GetWorld(), OtherActor->GetActorLocation(), 60.f, 12, FColor::Cyan, false, 3.f);
	}

	UCAbilitySystemStatics::ApplyTimedShield(OwnerASC, OtherASC, ShieldEffect, ShieldBuffIconEffect, AbilityLevel, ShieldDuration, ShieldAmount);
}

void AConsecratedGroundZone::Detonate()
{
	if ( bDetonated || !HasAuthority() )
		return;

	bDetonated = true;

	Multicast_PlayDetonationVFX();

	GetWorldTimerManager().ClearTimer(NaturalExpiryTimerHandle);

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere= FCollisionShape::MakeSphere(Radius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		Sphere,
		Params
	);

	if ( bShouldDrawDebug )
	{
		DrawDebugSphere(GetWorld(), GetActorLocation(), Radius, 24, FColor::Orange, false, 2.f, 0, 4.f);
		UE_LOG(LogTemp, Warning, TEXT("[ConsecratedGround] Detonate! Overlap 후보 수: %d"), Overlaps.Num());
	}

	TSet<AActor*> ProcessedTargets;

	for ( const FOverlapResult& Overlap : Overlaps )
	{
		AActor* OtherActor = Overlap.GetActor();
		if ( !OtherActor || ProcessedTargets.Contains(OtherActor) )
			continue;

		ProcessedTargets.Add(OtherActor);

		IGenericTeamAgentInterface* OtherTeamInterface = Cast<IGenericTeamAgentInterface>(OtherActor);
		if ( !OtherTeamInterface || OtherTeamInterface->GetGenericTeamId() == TeamId )
			continue;

		UAbilitySystemComponent* OtherASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
		if ( !OtherASC || !DamageEffect || !OwnerASC )
			continue;

		if ( bShouldDrawDebug )
		{
			UE_LOG(LogTemp, Warning, TEXT("[ConsecratedGround]   -> 데미지 대상: %s"), *OtherActor->GetName());
			DrawDebugSphere(GetWorld(), OtherActor->GetActorLocation(), 60.f, 12, FColor::Red, false, 3.f);
		}

		if ( ACharacter* OtherCharacter = Cast<ACharacter>(OtherActor) )
		{
			FVector PushDirection = (OtherActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();

			if ( PushDirection.IsNearlyZero() ) 
			{
				PushDirection = FVector::ForwardVector;
			}

			FVector PushVelocity = PushDirection * PushSpeed;
			PushVelocity.Z = PushSpeed * PushUpwardRatio;

			OtherCharacter->LaunchCharacter(PushVelocity, true, true);

			if ( bShouldDrawDebug )
			{
				DrawDebugLine(GetWorld(), GetActorLocation(), GetActorLocation() + PushDirection * Radius, FColor::Purple, false, 3.f, 0, 2.f);
			}
		}

		FGameplayEffectSpecHandle DamageSpec = OwnerASC->MakeOutgoingSpec(DamageEffect, AbilityLevel, OwnerASC->MakeEffectContext());
		if ( DamageSpec.IsValid() )
		{
			DamageSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetAbilityBaseDamageTag(), BaseDamage);
			DamageSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetAbilityScalingRatioTag(), ScalingRatio);
			OwnerASC->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), OtherASC);
		}
	}

	Destroy();
}

void AConsecratedGroundZone::Multicast_PlayDetonationVFX_Implementation()
{
	if ( DetonationVFX )
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DetonationVFX, GetActorLocation(), FRotator::ZeroRotator);
	}
}
