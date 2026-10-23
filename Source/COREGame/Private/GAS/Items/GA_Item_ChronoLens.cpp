// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Items/GA_Item_ChronoLens.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GameFramework/Character.h"
#include "Player/CPlayerCharacter.h"

UGA_Item_ChronoLens::UGA_Item_ChronoLens()
{
	ActivationOwnedTags.AddTag(UCAbilitySystemStatics::GetInvincibleStatusTag());
	ActivationOwnedTags.AddTag(UCAbilitySystemStatics::GetCCImmuneStatusTag());
}

void UGA_Item_ChronoLens::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if ( !HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo) )
	{
		K2_EndAbility();
		return;
	}

	if ( !K2_CommitAbility() )
	{
		K2_EndAbility();
		return;
	}

	if ( ACPlayerCharacter* CPlayerCharacter = Cast<ACPlayerCharacter>(GetOwningAvatarCharacter()) )
	{
		if(HasAuthority(&ActivationInfo) )
		{
			CPlayerCharacter->Multicast_SetChronoLensVisuals(true);
		}

		CPlayerCharacter->SetMoveInputBlock(true);
		CPlayerCharacter->SetBlockAbilityInput(true);
	}

	if ( HasAuthority(&ActivationInfo) )
	{
		GetWorld()->GetTimerManager().SetTimer(DurationTimerHandle, this, &UGA_Item_ChronoLens::K2_EndAbility, InvincibleDuration, false);
	}
}

void UGA_Item_ChronoLens::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	GetWorld()->GetTimerManager().ClearTimer(DurationTimerHandle);

	if ( ACPlayerCharacter* CPlayerCharacter = Cast<ACPlayerCharacter>(GetOwningAvatarCharacter()) )
	{
		if ( HasAuthority(&ActivationInfo) )
		{
			CPlayerCharacter->Multicast_SetChronoLensVisuals(false);
		}

		CPlayerCharacter->SetMoveInputBlock(false);
		CPlayerCharacter->SetBlockAbilityInput(false);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
