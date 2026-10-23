// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Muriel/GA_Muriel_ConsecratedGround.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "GAS/Muriel/ConsecratedGroundZone.h"
#include "GAS/Muriel/TA_ConsecratedGroundPreview.h"
#include "Widgets/GameplayWidget.h"
#include "Kismet/GameplayStatics.h"

void UGA_Muriel_ConsecratedGround::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if ( !HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo) )
	{
		K2_EndAbility();
		return;
	}

	if ( !K2_CheckAbilityCost() || !K2_CheckAbilityCooldown() )
	{
		K2_EndAbility();
		return;
	}

	UAbilityTask_PlayMontageAndWait* PlayTargetingMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, TargetingMontage);
	PlayTargetingMontage->ReadyForActivation();

	UAbilityTask_WaitTargetData* WaitTargetData = UAbilityTask_WaitTargetData::WaitTargetData(this, NAME_None, EGameplayTargetingConfirmation::UserConfirmed, PreviewTargetActorClass);
	WaitTargetData->ValidData.AddDynamic(this, &UGA_Muriel_ConsecratedGround::OnTargetConfirmed);
	WaitTargetData->Cancelled.AddDynamic(this, &UGA_Muriel_ConsecratedGround::OnTargetCancelled);
	WaitTargetData->ReadyForActivation();

	AGameplayAbilityTargetActor* TargetActor;
	WaitTargetData->BeginSpawningActor(this, PreviewTargetActorClass, TargetActor);
	if ( ATA_ConsecratedGroundPreview* PreviewActor = Cast<ATA_ConsecratedGroundPreview>(TargetActor) ) 
	{
		PreviewActor->ConfigurePreview(ZoneRadius, MaxRange);
	}
	WaitTargetData->FinishSpawningActor(this, TargetActor);

	AddSkillAimEffect();
}

void UGA_Muriel_ConsecratedGround::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	RemoveSkillAimEffect();

	if ( UGameplayWidget* Widget = GetOwningGameplayWidget() )
	{
		Widget->HideHoldProgress();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Muriel_ConsecratedGround::AddSkillAimEffect()
{
	if ( !SkillAimEffect || SkillAimEffectHandle.IsValid() )
	{
		return;
	}

	SkillAimEffectHandle = BP_ApplyGameplayEffectToOwner(SkillAimEffect);
}

void UGA_Muriel_ConsecratedGround::RemoveSkillAimEffect()
{
	if ( SkillAimEffectHandle.IsValid() )
	{
		BP_RemoveGameplayEffectFromOwnerWithHandle(SkillAimEffectHandle);
	}
}

void UGA_Muriel_ConsecratedGround::OnTargetConfirmed(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	if ( !K2_CommitAbility() )
	{
		K2_EndAbility();
		return;
	}

	RemoveSkillAimEffect();

	AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
	if ( !OwnerAvatarActor )
	{
		K2_EndAbility();
		return;
	}

	if ( K2_HasAuthority() || IsLocallyControlled() )
	{
		UAbilityTask_PlayMontageAndWait* PlayCastMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, CastMontage);
		PlayCastMontage->ReadyForActivation();
	}

	FHitResult TargetHit = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetDataHandle, 0);
	FVector SpawnLocation = TargetHit.Location;

	if ( K2_HasAuthority() && ZoneClass )
	{
		int32 AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);
		float Damage = GetLevelIndexedDamage(BaseDamagePerLevel, AbilityLevel);
		UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();

		FActorSpawnParameters SpawnParam;
		SpawnParam.Owner = OwnerAvatarActor;
		SpawnParam.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation);

		SpawnedZone = GetWorld()->SpawnActorDeferred<AConsecratedGroundZone>(ZoneClass, SpawnTransform, OwnerAvatarActor, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		if ( SpawnedZone )
		{
			SpawnedZone->ConfigureZone(ZoneRadius, ZoneDuration, GetOwnerTeamId(), OwnerASC, ShieldEffect, ShieldBuffIconEffect, ShieldDuration, DamageEffect, Damage, ScalingRatio, AbilityLevel);
			SpawnedZone->SetShieldAmount(GetScaledAmount(ShieldBaseAmountPerLevel, AbilityLevel, 0.f, ShieldIntelligenceRatio));
			UGameplayStatics::FinishSpawningActor(SpawnedZone, SpawnTransform); 
			
			SpawnedZone->ApplyVisualScale();
		}
	}

	if ( UGameplayWidget* Widget = GetOwningGameplayWidget() )
	{
		Widget->ShowHoldProgress(ZoneDuration, true);
	}
	
	UAbilityTask_WaitInputPress* WaitRetriggerTask = UAbilityTask_WaitInputPress::WaitInputPress(this);
	WaitRetriggerTask->OnPress.AddDynamic(this, &UGA_Muriel_ConsecratedGround::OnRetriggerInputPressed);
	WaitRetriggerTask->ReadyForActivation();

	if ( K2_HasAuthority() )
	{
		FTimerHandle AbilityEndTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(AbilityEndTimerHandle, this, &UGA_Muriel_ConsecratedGround::K2_EndAbility, ZoneDuration, false);
	}
}

void UGA_Muriel_ConsecratedGround::OnTargetCancelled(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	K2_EndAbility();
}

void UGA_Muriel_ConsecratedGround::OnRetriggerInputPressed(float TimeWaited)
{
	if ( K2_HasAuthority() && SpawnedZone )
	{
		SpawnedZone->DetonateEarly();
	}

	K2_EndAbility();
}