// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA_Dash.h"
#include "Abilities/Tasks/AbilityTask_NetworkSyncPoint.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Player/CPlayerCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/TargetActor_Around.h"
#include "GAS/TA_Directional.h"

UGA_Dash::UGA_Dash()
{
	ActivationOwnedTags.AddTag(UCAbilitySystemStatics::GetSkillAimStatusTag());
	ActivationOwnedTags.AddTag(UCAbilitySystemStatics::GetDisableJumpTag());
}

void UGA_Dash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if ( !K2_CheckAbilityCost() || !DashMontage )
		return;

	ACharacter* OwnerCharacter = GetOwningAvatarCharacter();
	if (!OwnerCharacter)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UCharacterMovementComponent* OwnerMovementComp = OwnerCharacter->GetCharacterMovement();
	if (!OwnerMovementComp)
	{
		K2_EndAbility();
		return;
	}

	UAbilityTask_PlayMontageAndWait* PlayCastMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, CastMontage);
	/*PlayCastMontage->OnBlendOut.AddDynamic(this, &UGA_Dash::K2_EndAbility);
	PlayCastMontage->OnInterrupted.AddDynamic(this, &UGA_Dash::K2_EndAbility);
	PlayCastMontage->OnCancelled.AddDynamic(this, &UGA_Dash::K2_EndAbility);
	PlayCastMontage->OnCompleted.AddDynamic(this, &UGA_Dash::K2_EndAbility);*/
	PlayCastMontage->ReadyForActivation();

	UAbilityTask_WaitTargetData* WaitDirectionTask = UAbilityTask_WaitTargetData::WaitTargetData(this, NAME_None, EGameplayTargetingConfirmation::UserConfirmed, DirectionTargetActorClass);
	WaitDirectionTask->ValidData.AddDynamic(this, &UGA_Dash::DirectionConfirmed);
	WaitDirectionTask->Cancelled.AddDynamic(this, &UGA_Dash::DirectionCancelled);
	WaitDirectionTask->ReadyForActivation();

	AGameplayAbilityTargetActor* TargetActor;
	WaitDirectionTask->BeginSpawningActor(this, DirectionTargetActorClass, TargetActor);
	ATA_Directional* DirectionTargetActor = Cast<ATA_Directional>(TargetActor);
	if ( DirectionTargetActor )
	{
		DirectionTargetActor->SetShouldDrawDebug(ShouldDrawDebug());
		DirectionTargetActor->SetDashDistance(DashDistance);
	}
	WaitDirectionTask->FinishSpawningActor(this, TargetActor);
}

void UGA_Dash::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (OwnerASC && DashEffectHandle.IsValid())
	{
		OwnerASC->RemoveActiveGameplayEffect(DashEffectHandle);
	}

	ACPlayerCharacter* CPlayerCharacter = Cast<ACPlayerCharacter>(GetOwningAvatarCharacter());
	if ( CPlayerCharacter )
	{
		CPlayerCharacter->SetMoveInputBlock(false);

		CPlayerCharacter->GetCharacterMovement()->bOrientRotationToMovement = bOriginalOrientRotationToMovement;
		CPlayerCharacter->bUseControllerRotationYaw = bOriginalUseControllerRotationYaw;
	}

	if (PushFowardInputTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(PushFowardInputTimerHandle);
	}

	if (OwnerCharacterMovementComponent)
	{
		OwnerCharacterMovementComponent->StopMovementImmediately();
		OwnerCharacterMovementComponent = nullptr;
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FGameplayTag UGA_Dash::GetDashStartTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Dash.Start");
}

void UGA_Dash::DirectionConfirmed(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	if ( !K2_CommitAbility() )
	{
		K2_EndAbility();
		return;
	}

	ACharacter* OwnerCharacter = GetOwningAvatarCharacter();
	if ( !OwnerCharacter )
	{
		K2_EndAbility();
		return;
	}

	FHitResult TargetHit = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetDataHandle, 0);
	FVector TargetPoint = TargetHit.ImpactPoint;

	DashStartLocation = OwnerCharacter->GetActorLocation();
	ConfirmedDashDirection = (TargetPoint - DashStartLocation);
	ConfirmedDashDirection.Z = 0.f;
	ConfirmedDashDirection = ConfirmedDashDirection.GetSafeNormal();

	if ( HasAuthorityOrPredictionKey(CurrentActorInfo, &CurrentActivationInfo) )
	{
		UAbilityTask_PlayMontageAndWait* PlayDashMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, DashMontage);
		PlayDashMontage->OnBlendOut.AddDynamic(this, &UGA_Dash::K2_EndAbility);
		PlayDashMontage->OnInterrupted.AddDynamic(this, &UGA_Dash::K2_EndAbility);
		PlayDashMontage->OnCancelled.AddDynamic(this, &UGA_Dash::K2_EndAbility);
		PlayDashMontage->OnCompleted.AddDynamic(this, &UGA_Dash::K2_EndAbility);
		PlayDashMontage->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* WaitDashStartEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetDashStartTag());
		WaitDashStartEvent->EventReceived.AddDynamic(this, &UGA_Dash::StartDash);
		WaitDashStartEvent->ReadyForActivation();
	}
}

void UGA_Dash::DirectionCancelled(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	K2_EndAbility();
}

void UGA_Dash::StartDash(FGameplayEventData Payload)
{
	if (K2_HasAuthority())
	{
		if (DashEffect)
		{
			DashEffectHandle = BP_ApplyGameplayEffectToOwner(DashEffect, GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo));
		}
	}

	if (IsLocallyControlled())
	{
		DashStartLocation = GetAvatarActorFromActorInfo()->GetActorLocation();
		PushFowardInputTimerHandle = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UGA_Dash::PushFoward);
		OwnerCharacterMovementComponent = GetAvatarActorFromActorInfo()->GetComponentByClass<UCharacterMovementComponent>();
	}

	if ( ACPlayerCharacter* CPlayerCharacter = Cast<ACPlayerCharacter>(GetOwningAvatarCharacter()) )
	{
		CPlayerCharacter->SetMoveInputBlock(true);

		bOriginalOrientRotationToMovement = CPlayerCharacter->GetCharacterMovement()->bOrientRotationToMovement;
		bOriginalUseControllerRotationYaw = CPlayerCharacter->bUseControllerRotationYaw;

		CPlayerCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
		CPlayerCharacter->bUseControllerRotationYaw = false;

		CPlayerCharacter->SetActorRotation(FRotator(0.f, ConfirmedDashDirection.Rotation().Yaw, 0.f));
	}

	UAbilityTask_WaitTargetData* WaitTargetData = UAbilityTask_WaitTargetData::WaitTargetData(this, NAME_None, EGameplayTargetingConfirmation::CustomMulti, TargetActorClass);
	WaitTargetData->ValidData.AddDynamic(this, &UGA_Dash::TargetReceived);
	WaitTargetData->ReadyForActivation();

	AGameplayAbilityTargetActor* TargetActor;
	WaitTargetData->BeginSpawningActor(this, TargetActorClass, TargetActor);
	ATargetActor_Around* TargetActorAround = Cast<ATargetActor_Around>(TargetActor);
	if (TargetActorAround)
	{
		TargetActorAround->ConfigureDetection(TargetDetectionRadius, GetOwnerTeamId(), LocalGameplayCueTag);
	}
	WaitTargetData->FinishSpawningActor(this, TargetActor);
	if (TargetActorAround)
	{
		TargetActorAround->AttachToComponent(GetOwningComponentFromActorInfo(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetActorAttachSocketName);
	}
}

void UGA_Dash::PushFoward()
{
	if ( !OwnerCharacterMovementComponent )
		return;

	AActor* Avatar = GetAvatarActorFromActorInfo();
	float TraveledDistance = FVector::Dist2D(Avatar->GetActorLocation(), DashStartLocation);

	if ( TraveledDistance >= DashDistance - 10.f )
	{
		K2_EndAbility();
		return;
	}

	OwnerCharacterMovementComponent->AddInputVector(ConfirmedDashDirection);

	PushFowardInputTimerHandle = GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UGA_Dash::PushFoward);
}

void UGA_Dash::TargetReceived(const FGameplayAbilityTargetDataHandle& Data)
{
	if (K2_HasAuthority())
	{
		int32 AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);
		float BaseDamage = GetLevelIndexedDamage(BaseDamagePerLevel, AbilityLevel);
		FGameplayEffectSpecHandle DamageSpec = MakeScaledDamageSpec(DamageEffect, BaseDamage, ScalingRatio, AbilityLevel);

		ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, DamageSpec, Data);
		PushTargetsFromOwnerLocation(Data, TargetHitPushSpeed, true);
	}
}