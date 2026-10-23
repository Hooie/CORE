// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA_Blackhole.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "GameFramework/Character.h"
#include "GAS/TA_Blackhole.h"
#include "GAS/TargetActor_GroundPick.h"
#include "GAS/CAbilitySystemStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/GameplayWidget.h"

UGA_Blackhole::UGA_Blackhole()
{
	SetAssetTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Type.Ultimate")));
}

void UGA_Blackhole::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		K2_EndAbility();
		return;
	}

	PlayCastBlackholeMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, TargetingMontage);
	PlayCastBlackholeMontageTask->OnBlendOut.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
	PlayCastBlackholeMontageTask->OnInterrupted.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
	PlayCastBlackholeMontageTask->OnCancelled.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
	PlayCastBlackholeMontageTask->OnCompleted.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
	PlayCastBlackholeMontageTask->ReadyForActivation();

	UAbilityTask_WaitTargetData* WaitPlacementTask = UAbilityTask_WaitTargetData::WaitTargetData(this, NAME_None, EGameplayTargetingConfirmation::UserConfirmed, TargetActorClass);
	WaitPlacementTask->ValidData.AddDynamic(this, &UGA_Blackhole::PlaceBlackhole);
	WaitPlacementTask->Cancelled.AddDynamic(this, &UGA_Blackhole::PlacementCancelled);
	WaitPlacementTask->ReadyForActivation();

	AGameplayAbilityTargetActor* TargetActor;
	WaitPlacementTask->BeginSpawningActor(this, TargetActorClass, TargetActor);
	ATargetActor_GroundPick* GroundPickTargetActor = Cast<ATargetActor_GroundPick>(TargetActor);
	if (GroundPickTargetActor)
	{
		GroundPickTargetActor->SetShouldDrawDebug(ShouldDrawDebug());
		GroundPickTargetActor->SetTargetAreaRadius(TargetAreaRadius);
		GroundPickTargetActor->SetTargetTraceRange(TargetTraceRange);
	}
	WaitPlacementTask->FinishSpawningActor(this, TargetActor);

	AddSkillAimEffect();
}

void UGA_Blackhole::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	RemoveSkillAimEffect();
	RemoveFocusEffect();

	if ( UGameplayWidget* Widget = GetOwningGameplayWidget() )
	{
		Widget->HideHoldProgress();
	}

	if ( HoldLoopVFXCueTag.IsValid() )
	{
		if ( UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo() )
		{
			OwnerASC->RemoveGameplayCue(HoldLoopVFXCueTag);
		}
	}

	if ( K2_HasAuthority() )
	{
		if ( UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo() )
		{
			OwnerASC->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetStunStatusTag()).RemoveAll(this);
		}
	}

	SpawnedBlackholeTargetActor = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Blackhole::PlaceBlackhole(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	if (!K2_CommitAbility())
	{
		K2_EndAbility();
		return;
	}

	RemoveSkillAimEffect();
	AddFocusEffect();

	if (PlayCastBlackholeMontageTask)
	{
		PlayCastBlackholeMontageTask->OnBlendOut.RemoveAll(this);
		PlayCastBlackholeMontageTask->OnInterrupted.RemoveAll(this);
		PlayCastBlackholeMontageTask->OnCancelled.RemoveAll(this);
		PlayCastBlackholeMontageTask->OnCompleted.RemoveAll(this);
	}

	if (HasAuthorityOrPredictionKey(CurrentActorInfo, &CurrentActivationInfo))
	{
		UAbilityTask_PlayMontageAndWait* PlayHoldBlackholeMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, HoldingBlackholeMontage);
		PlayHoldBlackholeMontage->OnBlendOut.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
		PlayHoldBlackholeMontage->OnInterrupted.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
		PlayHoldBlackholeMontage->OnCancelled.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
		PlayHoldBlackholeMontage->OnCompleted.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
		PlayHoldBlackholeMontage->ReadyForActivation();
	}

	if ( UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo() )
	{
		OwnerASC->AddGameplayCue(HoldLoopVFXCueTag);

		if ( K2_HasAuthority() )
			OwnerASC->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetStunStatusTag()).AddUObject(this, &UGA_Blackhole::OnStunTagChanged);
	}

	BlackholeTargetingTask = UAbilityTask_WaitTargetData::WaitTargetData(this, NAME_None, EGameplayTargetingConfirmation::UserConfirmed, BlackholeTargetActorClass);
	BlackholeTargetingTask->ValidData.AddDynamic(this, &UGA_Blackhole::FinalTargetReceived);
	BlackholeTargetingTask->Cancelled.AddDynamic(this, &UGA_Blackhole::FinalTargetReceived);
	BlackholeTargetingTask->ReadyForActivation();

	AGameplayAbilityTargetActor* TargetActor;
	BlackholeTargetingTask->BeginSpawningActor(this, BlackholeTargetActorClass, TargetActor);
	ATA_Blackhole* BlackholeTargetActor = Cast<ATA_Blackhole>(TargetActor);
	if (BlackholeTargetActor)
	{
		BlackholeTargetActor->ConfigureBlackhole(TargetAreaRadius, BlackholePullSpeed, BlackholeDuration, GetOwnerTeamId());
	}

	BlackholeTargetingTask->FinishSpawningActor(this, TargetActor);
	
	if (BlackholeTargetActor)
	{
		BlackholeTargetActor->SetActorLocation(UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetDataHandle, 1).ImpactPoint);
	}

	SpawnedBlackholeTargetActor = BlackholeTargetActor;

	BlackholeSpawnTime = GetWorld()->GetTimeSeconds();

	if ( UGameplayWidget* Widget = GetOwningGameplayWidget() )
	{
		Widget->ShowHoldProgress(BlackholeDuration);
	}
}

void UGA_Blackhole::PlacementCancelled(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	K2_EndAbility();
}

void UGA_Blackhole::FinalTargetReceived(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	if ( UGameplayWidget* Widget = GetOwningGameplayWidget() )
	{
		Widget->HideHoldProgress();
	}


	if ( UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo() )
	{
		OwnerASC->RemoveGameplayCue(HoldLoopVFXCueTag);
	}

	if ( K2_HasAuthority() )
	{
		if ( UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo() )
		{
			OwnerASC->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetStunStatusTag()).RemoveAll(this);
		}
	}

	SpawnedBlackholeTargetActor = nullptr;

	if (K2_HasAuthority())
	{
		float ElapsedHoldTime = GetWorld()->GetTimeSeconds() - BlackholeSpawnTime;
		float HoldRatio = BlackholeDuration > 0.f ? FMath::Clamp(ElapsedHoldTime / BlackholeDuration, 0.f, 1.f) : 1.f;

		float DamageMultiplier = FMath::Lerp(MinHoldDamageMultiplier, MaxHoldDamageMultiplier, HoldRatio);

		int32 AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);
		float BaseDamage = GetLevelIndexedDamage(BaseDamagePerLevel, AbilityLevel) * DamageMultiplier;
		float FinalScalingRatio = ScalingRatio * DamageMultiplier;

		FGameplayEffectSpecHandle DamageSpec = MakeScaledDamageSpec(FinalBlowDamageEffect, BaseDamage, FinalScalingRatio, AbilityLevel);
		ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, DamageSpec, TargetDataHandle);

		FVector BlowCenter = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetDataHandle, 1).ImpactPoint;
		PushTargetsFromLocation(TargetDataHandle, BlowCenter, BlowPushSpeed, true);

		UAbilityTask_PlayMontageAndWait* PlayFinalBlowMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, FinalBlowMontage);
		PlayFinalBlowMontage->OnBlendOut.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
		PlayFinalBlowMontage->OnInterrupted.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
		PlayFinalBlowMontage->OnCancelled.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
		PlayFinalBlowMontage->OnCompleted.AddDynamic(this, &UGA_Blackhole::K2_EndAbility);
		PlayFinalBlowMontage->ReadyForActivation();
	}
	else
	{
		PlayMontageLocally(FinalBlowMontage);
	}

	FGameplayCueParameters FinalBlowCueParams;
	FinalBlowCueParams.Location = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetDataHandle, 1).ImpactPoint;
	FinalBlowCueParams.RawMagnitude = TargetAreaRadius;

	GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(FinalBlowCueTag, FinalBlowCueParams);
	GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(UCAbilitySystemStatics::GetCameraShakeGameplayCueTag(), FinalBlowCueParams);
}

void UGA_Blackhole::AddSkillAimEffect()
{
	SkillAimEffectHandle = BP_ApplyGameplayEffectToOwner(SkillAimEffect);
}

void UGA_Blackhole::RemoveSkillAimEffect()
{
	if ( SkillAimEffectHandle.IsValid())
	{
		BP_RemoveGameplayEffectFromOwnerWithHandle(SkillAimEffectHandle);
	}
}

void UGA_Blackhole::AddFocusEffect()
{
	FocusEffectHandle = BP_ApplyGameplayEffectToOwner(FocusEffect);
}

void UGA_Blackhole::RemoveFocusEffect()
{
	if (FocusEffectHandle.IsValid())
	{
		BP_RemoveGameplayEffectFromOwnerWithHandle(FocusEffectHandle);
	} 
}

void UGA_Blackhole::OnStunTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if ( NewCount <= 0 )
		return;

	if ( IsValid(SpawnedBlackholeTargetActor) )
	{
		SpawnedBlackholeTargetActor->ForceStop(); 
	}
}

