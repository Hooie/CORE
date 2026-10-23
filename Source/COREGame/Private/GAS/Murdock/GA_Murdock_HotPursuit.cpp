// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Murdock/GA_Murdock_HotPursuit.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/CAttributeSet.h"
#include "GenericTeamAgentInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

UGA_Murdock_HotPursuit::UGA_Murdock_HotPursuit()
{
	SetAssetTags(FGameplayTagContainer(UCAbilitySystemStatics::GetPassiveAbilityTypeTag()));

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_Murdock_HotPursuit::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if ( !HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo) )
	{
		K2_EndAbility();
		return;
	}

	int32 AbilityLevel = GetAbilityLevel(Handle, ActorInfo);
	if ( AbilityLevel <= 0 )
		return;

	CurrentAbilityLevel = AbilityLevel;
	StartPeriodicCheck();
}

void UGA_Murdock_HotPursuit::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	StopPeriodicCheck();

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if ( ASC )
	{
		ASC->RemoveActiveGameplayEffect(EnhanceMovementEffectHandle);
		EnhanceMovementEffectHandle.Invalidate();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Murdock_HotPursuit::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if ( ASC )
	{
		ASC->AbilitySpecDirtiedCallbacks.AddUObject(this, &UGA_Murdock_HotPursuit::OnAbilitySpecDirted);
	}
}

void UGA_Murdock_HotPursuit::OnAbilitySpecDirted(const FGameplayAbilitySpec& Spec)
{
	if ( Spec.Ability != GetClass()->GetDefaultObject<UGameplayAbility>() && Spec.Ability != this )
	{
		return;
	}

	if ( Spec.Level >= 1 )
	{
		CurrentAbilityLevel = Spec.Level;

		if ( !GetWorld()->GetTimerManager().IsTimerActive(CheckTimerHandle) )
		{
			StartPeriodicCheck();
		}
	}
}

void UGA_Murdock_HotPursuit::StartPeriodicCheck()
{
	GetWorld()->GetTimerManager().SetTimer(CheckTimerHandle, this, &UGA_Murdock_HotPursuit::PeriodicCheck, CheckInterval, true);
}

void UGA_Murdock_HotPursuit::StopPeriodicCheck()
{
	GetWorld()->GetTimerManager().ClearTimer(CheckTimerHandle);
}

void UGA_Murdock_HotPursuit::PeriodicCheck()
{
	AActor* OwnerAvatarActor= GetAvatarActorFromActorInfo();
	ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerAvatarActor);
	if ( !OwnerCharacter )
		return;

	UCharacterMovementComponent* MovementComp = OwnerCharacter->GetCharacterMovement();
	FVector Velocity = MovementComp ? MovementComp->Velocity : FVector::ZeroVector;

	bool bShouldBuff = false;

	if ( !Velocity.IsNearlyZero() )
	{
		float Radius = GetLevelIndexedDamage(DetectionRadiusPerLevel, CurrentAbilityLevel);

		FVector Enemylocation;
		if ( FindQualifyingLowHealthEnemy(Radius, Enemylocation) )
		{
			FVector MoveDir = Velocity.GetSafeNormal2D();
			FVector DirToEnemy = (Enemylocation - OwnerAvatarActor->GetActorLocation()).GetSafeNormal2D();

			if ( FVector::DotProduct(MoveDir, DirToEnemy) >= MovingTowardDotThreshold )
			{
				bShouldBuff = true;
			}
		}
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if ( !ASC )
		return;

	if ( bShouldBuff && !EnhanceMovementEffectHandle.IsValid() )
	{
		EnhanceMovementEffectHandle = BP_ApplyGameplayEffectToOwner(EnhanceMovementEffect, CurrentAbilityLevel);
	}
	else if ( !bShouldBuff && EnhanceMovementEffectHandle.IsValid() )
	{
		ASC->RemoveActiveGameplayEffect(EnhanceMovementEffectHandle);
		EnhanceMovementEffectHandle.Invalidate();
	}
}

bool UGA_Murdock_HotPursuit::FindQualifyingLowHealthEnemy(float Radius, FVector& OutEnemyLocation) const
{
	AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
	if ( !OwnerAvatarActor )
		return false;

	IGenericTeamAgentInterface* OwnerTeamInterface = Cast<IGenericTeamAgentInterface>(OwnerAvatarActor);
	if ( !OwnerTeamInterface )
		return false;

	FVector OwnerLocation = OwnerAvatarActor->GetActorLocation();

	TArray<FOverlapResult> Overlaps;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerAvatarActor);

	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		OwnerLocation,
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		Sphere,
		Params
	);

	for ( const FOverlapResult& Overlap : Overlaps )
	{
		AActor* OtherActor = Overlap.GetActor();
		if ( !OtherActor || !UCAbilitySystemStatics::IsHero(OtherActor) )
			continue;

		IGenericTeamAgentInterface* OtherTeamInterface = Cast<IGenericTeamAgentInterface>(OtherActor);
		if ( !OtherTeamInterface )
			continue;

		UAbilitySystemComponent* OtherASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
		if ( !OtherASC )
			continue;

		bool bFound = false;
		float Health = OtherASC->GetGameplayAttributeValue(UCAttributeSet::GetHealthAttribute(), bFound);
		float MaxHealth = OtherASC->GetGameplayAttributeValue(UCAttributeSet::GetMaxHealthAttribute(), bFound);

		if ( MaxHealth <= 0.f || (Health / MaxHealth) >= LowHealthThresholdPercent )
			continue;

		OutEnemyLocation = OtherActor->GetActorLocation();
		return true;
	}

	return false;
}
