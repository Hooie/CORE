// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA_Shoot.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameplayTagsManager.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/ProjectileActor.h"
#include "GAS/MMC_AbilityDamage.h"

UGA_Shoot::UGA_Shoot()
{
	ActivationOwnedTags.AddTag(UCAbilitySystemStatics::GetBasicAttackAimStatusTag());
	ActivationOwnedTags.AddTag(UCAbilitySystemStatics::GetCrosshairTag());

	BlockAbilitiesWithTag.AddTag(UCAbilitySystemStatics::GetDisarmedTag());
	CancelAbilitiesWithTag.AddTag(UCAbilitySystemStatics::GetDisarmedTag());
}

void UGA_Shoot::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!K2_CommitAbility())
	{
		K2_EndAbility();
		return;
	}

	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		UAbilityTask_WaitGameplayEvent* WaitStartShootingEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, UCAbilitySystemStatics::GetBasicAttackInputPressedTag());
		WaitStartShootingEvent->EventReceived.AddDynamic(this, &UGA_Shoot::StartShooting);
		WaitStartShootingEvent->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* WaitStopShootingEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, UCAbilitySystemStatics::GetBasicAttackInputReleasedTag());
		WaitStopShootingEvent->EventReceived.AddDynamic(this, &UGA_Shoot::StopShooting);
		WaitStopShootingEvent->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* WaitShootProjectileEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetShootTag(), nullptr, false, false);
		WaitShootProjectileEvent->EventReceived.AddDynamic(this, &UGA_Shoot::ShootProjectile);
		WaitShootProjectileEvent->ReadyForActivation();
	}
}

void UGA_Shoot::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	K2_EndAbility();
}

void UGA_Shoot::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (AimTargetAbilitySystemComponent)
	{
		AimTargetAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetDeadStatusTag()).RemoveAll(this);
		AimTargetAbilitySystemComponent = nullptr;
	}

	AimTarget = nullptr;

	SendLocalGameplayEvent(UCAbilitySystemStatics::GetTargetUpdatedTag(), FGameplayEventData());

	StopShooting(FGameplayEventData());

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UGA_Shoot::GetBasicAttackDamagePreview(const UAbilitySystemComponent* SourceASC, float& OutRawDamage, bool& bOutIsMagical) const
{
	return UMMC_AbilityDamage::PreviewRawDamage(ProjectileHitEffect, SourceASC, BaseDamage, 0.f, StrengthRatio, IntelligenceRatio, OutRawDamage, bOutIsMagical);
}

FGameplayTag UGA_Shoot::GetShootTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Shoot");
}

void UGA_Shoot::StartShooting(FGameplayEventData PayLoad)
{
	if (HasAuthority(&CurrentActivationInfo))
	{
		UAbilityTask_PlayMontageAndWait* PlayShootMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ShootMontage);
		PlayShootMontage->ReadyForActivation();
	}
	else
	{
		PlayMontageLocally(ShootMontage);
	}

	FindAimTarget();
	StartAimTargetCheckTimer();
}

void UGA_Shoot::StopShooting(FGameplayEventData PayLoad)
{
	if (ShootMontage)
	{
		StopMontageAfterCurrentSection(ShootMontage);
	}

	StopAimTargetCheckTimer();
}

void UGA_Shoot::ShootProjectile(FGameplayEventData PayLoad)
{
	if (K2_HasAuthority())
	{
		AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = OwnerAvatarActor;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		FVector SocketLocation = GetAvatarActorFromActorInfo()->GetActorLocation();
		USkeletalMeshComponent* MeshComp = GetOwningComponentFromActorInfo();
		if (MeshComp)
		{
			TArray<FName> OutNames;
			UGameplayTagsManager::Get().SplitGameplayTagFName(PayLoad.EventTag, OutNames);
			if (OutNames.Num() != 0)
			{
				FName SocketName = OutNames.Last();
				SocketLocation = MeshComp->GetSocketLocation(SocketName);
			}
		}

		AActor* CurrentAimTarget = GetAimTargetIfValid();

		FRotator LaunchRotation = OwnerAvatarActor->GetActorRotation();
		if ( !CurrentAimTarget )
		{
			if ( ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerAvatarActor) )
			{
				if ( UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent() )
				{
					LaunchRotation = Capsule->GetComponentRotation();
				}
			}
		}

		FGameplayEffectSpecHandle DamageSpec = MakeScaledDamageSpec(ProjectileHitEffect, BaseDamage, StrengthRatio, IntelligenceRatio, GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo));
		AProjectileActor* Projectile = GetWorld()->SpawnActor<AProjectileActor>(ProjectileClass, SocketLocation, LaunchRotation, SpawnParams);
		if ( Projectile )
		{
			Projectile->ShootProjectile(ShootProjectileSpeed, ShootProjectileRange, CurrentAimTarget, GetOwnerTeamId(), DamageSpec);
		}
	}
}

AActor* UGA_Shoot::GetAimTargetIfValid() const
{
	if (HasValidTarget())
		return AimTarget;

	return nullptr;
}

void UGA_Shoot::FindAimTarget()
{
	if (HasValidTarget())
		return;

	if (AimTargetAbilitySystemComponent)
	{
		AimTargetAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetDeadStatusTag()).RemoveAll(this);
		AimTargetAbilitySystemComponent = nullptr;
	}

	AimTarget = GetAimTarget(ShootProjectileRange, ETeamAttitude::Hostile);
	if (AimTarget)
	{
		AimTargetAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AimTarget);
		if (AimTargetAbilitySystemComponent)
		{
			AimTargetAbilitySystemComponent->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetDeadStatusTag()).AddUObject(this, &UGA_Shoot::TargetDeadTagUpdated);
		}
	}

	FGameplayEventData EventData;
	EventData.Target = AimTarget;

	SendLocalGameplayEvent(UCAbilitySystemStatics::GetTargetUpdatedTag(), EventData);
}

void UGA_Shoot::StartAimTargetCheckTimer()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(AimTargetCheckTimerHandle, this, &UGA_Shoot::FindAimTarget, AimTargetCheckInterval, true);
	}
}

void UGA_Shoot::StopAimTargetCheckTimer()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(AimTargetCheckTimerHandle);
	}
}

bool UGA_Shoot::HasValidTarget() const
{
	if (!AimTarget)
		return false;

	if (UCAbilitySystemStatics::IsActorDead(AimTarget))
		return false;

	if (!IsTargetInRange())
		return false;

	return true;
}

bool UGA_Shoot::IsTargetInRange() const
{
	if (!AimTarget)
		return false;

	float Distance = FVector::Distance(AimTarget->GetActorLocation(), GetAvatarActorFromActorInfo()->GetActorLocation());

	return Distance <= ShootProjectileRange;
}

void UGA_Shoot::TargetDeadTagUpdated(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		FindAimTarget();
	}
}
