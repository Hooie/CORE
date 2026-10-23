// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA_Combo.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "GameplayTagsManager.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/CAttributeSet.h"
#include "GAS/CHeroAttributeSet.h"
#include "GAS/Items/CItemStatics.h"
#include "GAS/MMC_AbilityDamage.h"

UGA_Combo::UGA_Combo()
{
	SetAssetTags(FGameplayTagContainer(UCAbilitySystemStatics::GetBasicAttackAbilityTag()));
	//AbilityTags.AddTag(UCAbilitySystemStatics::GetBasicAttackAbilityTag());

	BlockAbilitiesWithTag.AddTag(UCAbilitySystemStatics::GetBasicAttackAbilityTag());
	BlockAbilitiesWithTag.AddTag(UCAbilitySystemStatics::GetDisarmedTag());

	CancelAbilitiesWithTag.AddTag(UCAbilitySystemStatics::GetDisarmedTag());
}

void UGA_Combo::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!K2_CommitAbility() || IsBound() )
	{
		K2_EndAbility();
		return;
	}

	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		UAbilityTask_PlayMontageAndWait* PlayComboMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ComboMontage);
		PlayComboMontageTask->OnBlendOut.AddDynamic(this, &UGA_Combo::K2_EndAbility);
		PlayComboMontageTask->OnCancelled.AddDynamic(this, &UGA_Combo::K2_EndAbility);
		PlayComboMontageTask->OnCompleted.AddDynamic(this, &UGA_Combo::K2_EndAbility);
		PlayComboMontageTask->OnInterrupted.AddDynamic(this, &UGA_Combo::K2_EndAbility);
		PlayComboMontageTask->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* WaitComboChangeEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetComboChangedEventTag(), nullptr, false, false);
		WaitComboChangeEventTask->EventReceived.AddDynamic(this, &UGA_Combo::ComboChangedEventReceived);
		WaitComboChangeEventTask->ReadyForActivation();
	}

	if (K2_HasAuthority())
	{
		UAbilityTask_WaitGameplayEvent* WaitTargetingEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetComboTargetEventTag());
		WaitTargetingEventTask->EventReceived.AddDynamic(this, &UGA_Combo::DoDamage);
		WaitTargetingEventTask->ReadyForActivation();
	}

	NextComboName = NAME_None;
	SetupWaitComboInputPress();
}

FGameplayTag UGA_Combo::GetComboChangedEventTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Combo.Change");
}

FGameplayTag UGA_Combo::GetComboChangedEventEndTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Combo.Change.End");
}

FGameplayTag UGA_Combo::GetComboTargetEventTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Combo.Damage");
}

bool UGA_Combo::GetBasicAttackDamagePreview(const UAbilitySystemComponent* SourceASC, float& OutRawDamage, bool& bOutIsMagical) const
{
	return UMMC_AbilityDamage::PreviewRawDamage(DamageEffect, SourceASC, BaseDamage, ScalingRatio, 0.f, 0.f, OutRawDamage, bOutIsMagical);
}

void UGA_Combo::SetupWaitComboInputPress()
{
	UAbilityTask_WaitInputPress* WaitInputPress = UAbilityTask_WaitInputPress::WaitInputPress(this);

	WaitInputPress->OnPress.AddDynamic(this, &UGA_Combo::HandleInputPress);
	WaitInputPress->ReadyForActivation();
}

void UGA_Combo::HandleInputPress(float TimeWaited)
{
	SetupWaitComboInputPress();
	TryCommitCombo();
}

void UGA_Combo::TryCommitCombo()
{
	if ( IsBound() )
		return;

	if (NextComboName == NAME_None)
	{
		return;
	}

	UAnimInstance* OwnerAnimInst = GetOwnerAnimInstance();
	if (!OwnerAnimInst)
		return;

	OwnerAnimInst->Montage_SetNextSection(OwnerAnimInst->Montage_GetCurrentSection(ComboMontage), NextComboName, ComboMontage);
}

void UGA_Combo::ComboChangedEventReceived(FGameplayEventData Data)
{
	FGameplayTag EventTag = Data.EventTag;
	
	if (EventTag == GetComboChangedEventEndTag())
	{
		NextComboName = NAME_None;
		return;
	}

	TArray<FName> TagNames;
	UGameplayTagsManager::Get().SplitGameplayTagFName(EventTag, TagNames);

	NextComboName = TagNames.Last();
}

void UGA_Combo::DoDamage(FGameplayEventData Data)
{
	if ( !K2_HasAuthority() )
		return;

	int HitResultCount = UAbilitySystemBlueprintLibrary::GetDataCountFromTargetData(Data.TargetData);

	int32 AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);
	FGameplayEffectSpecHandle DamageSpec = MakeScaledDamageSpec(DamageEffect, BaseDamage, ScalingRatio, AbilityLevel);

	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();

	UCItemStatics::BeginLifeStealBatch(OwnerASC);

	TSet<AActor*> HitActors;

	for ( int i = 0; i < HitResultCount; ++i )
	{
		FHitResult HitResult = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(Data.TargetData, i);

		AActor* HitActor = HitResult.GetActor();
		if ( !HitActor || HitActors.Contains(HitActor) )
			continue;

		HitActors.Add(HitActor); 

		ApplyGameplayEffectToHitResultActor(HitResult, DamageSpec);
	}

	UCItemStatics::EndLifeStealBatch(OwnerASC);
}

void UGA_Combo::OnLifeStealProcess(FGameplayEventData EventData)
{

	if (!K2_HasAuthority())
	{
		return;
	}

	if (!GetLifeStealHealEffect())
	{
		return;
	}

	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if (!OwnerASC)
	{
		return;
	}

	bool bFound = false;
	float Ratio = OwnerASC->GetGameplayAttributeValue(UCHeroAttributeSet::GetLifeStealRatioAttribute(), bFound);

	if (!bFound || Ratio <= 0.f)
		return;

	float HealAmount = EventData.EventMagnitude * Ratio;

	if (HealAmount <= 0.f)
		return;

	FGameplayEffectSpecHandle HealSpec = MakeOutgoingGameplayEffectSpec(GetLifeStealHealEffect(), 1);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(HealSpec, UCAbilitySystemStatics::GetHealMagnitudeTag(), HealAmount);
	OwnerASC->ApplyGameplayEffectSpecToSelf(*HealSpec.Data.Get());
}

bool UGA_Combo::IsBound() const
{
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	
	return OwnerASC->HasMatchingGameplayTag(UCAbilitySystemStatics::GetDisarmedTag());
}