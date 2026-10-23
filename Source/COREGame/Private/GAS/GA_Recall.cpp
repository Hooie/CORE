// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA_Recall.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitMovementModeChange.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "Framework/CGameState.h"
#include "GenericTeamAgentInterface.h"
#include "Player/CPlayerCharacter.h"
#include "Widgets/GameplayWidget.h"

UGA_Recall::UGA_Recall()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FGameplayTagContainer OwnedTags;
	OwnedTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Type.Recalling"));
	SetAssetTags(OwnedTags);
}

void UGA_Recall::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if ( ACharacter* OwnerCharacter = GetOwningAvatarCharacter() )
	{
		if ( OwnerCharacter->GetCharacterMovement()->IsFalling() )
		{
			K2_EndAbility();
			return;
		}
	}

	if ( !K2_CommitAbility() || !RecallMontage )
	{
		K2_EndAbility();
		return;
	}

	if ( HasAuthorityOrPredictionKey(CurrentActorInfo, &CurrentActivationInfo) )
	{
		UAbilityTask_PlayMontageAndWait* PlayRecallMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, RecallMontage);
		PlayRecallMontage->OnInterrupted.AddDynamic(this, &UGA_Recall::OnRecallInterrupted);
		PlayRecallMontage->OnCancelled.AddDynamic(this, &UGA_Recall::OnRecallInterrupted);
		PlayRecallMontage->ReadyForActivation();
	}

	if ( K2_HasAuthority() )
	{
		UAbilityTask_WaitGameplayEvent* WaitDamagedEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, UCAbilitySystemStatics::GetDamagedEventTag(), nullptr, false);
		WaitDamagedEvent->EventReceived.AddDynamic(this, &UGA_Recall::OnDamagedDuringRecall);
		WaitDamagedEvent->ReadyForActivation();

		GetWorld()->GetTimerManager().SetTimer(RecallCompletionTimerHandle, this, &UGA_Recall::OnRecallTimerCompleted, RecallDuration, false);
	}

	if ( K2_HasAuthority() && RecallEffect )
	{
		RecallEffectHandle = BP_ApplyGameplayEffectToOwner(RecallEffect, GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo));
	}

	if ( UGameplayWidget* Widget = GetOwningGameplayWidget() )
	{
		Widget->ShowHoldProgress(RecallDuration, true);
	}
}

void UGA_Recall::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	GetWorld()->GetTimerManager().ClearTimer(RecallCompletionTimerHandle);
	
	if ( K2_HasAuthority() && RecallEffectHandle.IsValid() )
	{
		if ( UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo() )
		{
			OwnerASC->RemoveActiveGameplayEffect(RecallEffectHandle);
		}
	}

	if ( UGameplayWidget* Widget = GetOwningGameplayWidget() )
	{
		Widget->HideHoldProgress();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Recall::OnRecallTimerCompleted()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	IGenericTeamAgentInterface* TeamInterface = Cast<IGenericTeamAgentInterface>(Avatar);
	ACGameState* CGameState = GetWorld()->GetGameState<ACGameState>();

	if ( Avatar && TeamInterface && CGameState )
	{
		FVector BaseLocation;
		FRotator BaseRotation;
		if ( CGameState->GetBaseLocationForTeam(TeamInterface->GetGenericTeamId().GetId(), BaseLocation, BaseRotation) )
		{
			Avatar->SetActorLocation(BaseLocation, false, nullptr, ETeleportType::TeleportPhysics);
			Avatar->SetActorRotation(BaseRotation, ETeleportType::TeleportPhysics);

			if ( RecallCompletedVFXCueTag.IsValid() )
			{
				FGameplayCueParameters CueParams;
				CueParams.Location = BaseLocation;
				CueParams.Normal = FVector::UpVector;

				GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(RecallCompletedVFXCueTag, CueParams);
			}
		}
	}

	K2_EndAbility();
}

void UGA_Recall::OnRecallInterrupted()
{
	K2_EndAbility();
}

void UGA_Recall::OnDamagedDuringRecall(FGameplayEventData Payload)
{
	CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}