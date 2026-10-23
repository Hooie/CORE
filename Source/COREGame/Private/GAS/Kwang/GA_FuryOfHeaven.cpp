// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Kwang/GA_FuryOfHeaven.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/CKwangCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/Kwang/TA_Sword.h"
#include "GenericTeamAgentInterface.h"
#include "Kismet/KismetSystemLibrary.h"

UGA_FuryOfHeaven::UGA_FuryOfHeaven()
{
	SetAssetTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Type.Ultimate")));

	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Kwang.Fury.Disabled"));
}

void UGA_FuryOfHeaven::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if ( !HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo) )
		return;

	if ( !K2_CommitAbility() )
	{
		K2_EndAbility();
		return;
	}

	ACKwangCharacter* Kwang = Cast<ACKwangCharacter>(GetAvatarActorFromActorInfo());
	Kwang->SetMoveInputBlock(true);
	
	UAbilityTask_PlayMontageAndWait* PlayIntroMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, IntroMontage);
	PlayIntroMontage->OnInterrupted.AddDynamic(this, &UGA_FuryOfHeaven::K2_EndAbility);
	PlayIntroMontage->OnCancelled.AddDynamic(this, &UGA_FuryOfHeaven::K2_EndAbility);
	PlayIntroMontage->OnCompleted.AddDynamic(this, &UGA_FuryOfHeaven::IntroMontageCompleted);
	PlayIntroMontage->OnBlendOut.AddDynamic(this, &UGA_FuryOfHeaven::IntroMontageCompleted);
	PlayIntroMontage->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* WaitRecallSwordEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, UCAbilitySystemStatics::GetRecallSwordEventTag());
	WaitRecallSwordEvent->EventReceived.AddDynamic(this, &UGA_FuryOfHeaven::RecallSwordEventReceived);
	WaitRecallSwordEvent->ReadyForActivation();
}

void UGA_FuryOfHeaven::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ACKwangCharacter* Kwang = Cast<ACKwangCharacter>(GetAvatarActorFromActorInfo());
	Kwang->SetMoveInputBlock(false);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_FuryOfHeaven::IntroMontageCompleted()
{
	if ( K2_HasAuthority() && bShouldTeleport )
	{
		ACKwangCharacter* Kwang = Cast<ACKwangCharacter>(GetAvatarActorFromActorInfo());
		if ( Kwang )
		{
			SavedSwordLocation.Z += Kwang->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			Kwang->SetActorLocation(SavedSwordLocation);
		}
		bShouldTeleport = false;
	}

	if ( HasAuthorityOrPredictionKey(CurrentActorInfo, &CurrentActivationInfo) )
	{
		UAbilityTask_PlayMontageAndWait* PlayFinalMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, FinalMontage);
		PlayFinalMontage->OnBlendOut.AddDynamic(this, &UGA_FuryOfHeaven::K2_EndAbility);
		PlayFinalMontage->OnCancelled.AddDynamic(this, &UGA_FuryOfHeaven::K2_EndAbility);
		PlayFinalMontage->OnInterrupted.AddDynamic(this, &UGA_FuryOfHeaven::K2_EndAbility);
		PlayFinalMontage->OnCompleted.AddDynamic(this, &UGA_FuryOfHeaven::K2_EndAbility);
		PlayFinalMontage->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* WaitFinalDamageEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, UCAbilitySystemStatics::GetActivateEventTag());
		WaitFinalDamageEvent->EventReceived.AddDynamic(this, &UGA_FuryOfHeaven::FinalDamageEventReceived);
		WaitFinalDamageEvent->ReadyForActivation();
	}
	else
	{
		PlayMontageLocally(FinalMontage);
	}
}

void UGA_FuryOfHeaven::RecallSwordEventReceived(FGameplayEventData Payload)
{
	if ( !K2_HasAuthority() )
		return;

	if ( IsSwordInWorld() )
	{
		ACKwangCharacter* Kwang = Cast<ACKwangCharacter>(GetAvatarActorFromActorInfo());
		if ( Kwang )
		{
			ATA_Sword* Sword = Kwang->GetSpawnedSword();

			if ( IsValid(Sword) )
			{
				SavedSwordLocation = Sword->GetActorLocation();
				bShouldTeleport = true;

				Sword->ForceStop();
			}

			UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
			if ( OwnerASC )
			{
				FGameplayTagContainer JudgementTag;
				JudgementTag.AddTag(FGameplayTag::RequestGameplayTag("Ability.Kwang.Judgement"));
				OwnerASC->CancelAbilities(&JudgementTag);

			}

			Kwang->Multicast_ToggleSwordVisibility(true);
		}
	}
}

void UGA_FuryOfHeaven::FinalDamageEventReceived(FGameplayEventData Payload)
{
	if ( !K2_HasAuthority() )
		return;

	AActor* Avatar = GetAvatarActorFromActorInfo();
	if ( !Avatar )
		return;
	
	IGenericTeamAgentInterface* OwnerTeamInterface = Cast< IGenericTeamAgentInterface>(Avatar);
	if ( !OwnerTeamInterface )
		return;

	EDrawDebugTrace::Type DrawDebug = ShouldDrawDebug() ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	TArray<FHitResult> HitResults;

	UKismetSystemLibrary::SphereTraceMultiForObjects(
		GetWorld(),
		Avatar->GetActorLocation(),
		Avatar->GetActorLocation(),
		AttackRadius,
		TArray<TEnumAsByte<EObjectTypeQuery>>{UEngineTypes::ConvertToObjectType(ECC_Pawn)},
		false,
		TArray<AActor*>{Avatar},
		DrawDebug,
		HitResults,
		true
	);

	int32 AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);
	float BaseDamage = GetLevelIndexedDamage(BaseDamagePerLevel, AbilityLevel);

	FGameplayEffectSpecHandle DamageSpec = MakeScaledDamageSpec(DamageEffect, BaseDamage, ScalingRatio, AbilityLevel);

	for ( const FHitResult& HitResult : HitResults )
	{
		if ( OwnerTeamInterface->GetTeamAttitudeTowards(*(HitResult.GetActor())) == ETeamAttitude::Hostile )
		{
			ApplyGameplayEffectToHitResultActor(HitResult, DamageSpec);
		}
	}
	
	BP_ApplyGameplayEffectToOwner(BuffEffect, GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo));
}

bool UGA_FuryOfHeaven::IsSwordInWorld()
{
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if ( !OwnerASC )
		return false;

	return OwnerASC->HasMatchingGameplayTag(UCAbilitySystemStatics::GetNoWeaponTag());
}

void UGA_FuryOfHeaven::TeleportToSword()
{
	if ( !K2_HasAuthority() )
		return;

	ACKwangCharacter* Kwang = Cast<ACKwangCharacter>(GetAvatarActorFromActorInfo());
	if ( !Kwang )
		return;

	ATA_Sword* Sword = Kwang->GetSpawnedSword();
	if ( IsValid(Sword) )
	{
		FVector TeleportLoc = Sword->GetActorLocation();
		TeleportLoc.Z += Kwang->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Kwang->SetActorLocation(TeleportLoc);
	}
}
