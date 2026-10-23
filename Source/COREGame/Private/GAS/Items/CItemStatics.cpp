// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Items/CItemStatics.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AI/Turret/Turret.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/CAttributeSet.h"
#include "GAS/CGameplayEffectSettings.h"
#include "GAS/CHeroAttributeSet.h"
#include "GenericTeamAgentInterface.h"

TMap<TWeakObjectPtr<UAbilitySystemComponent>, FPendingLifeSteal> UCItemStatics::PendingLifeSteal;

UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Passive_StarterBlade, "Item.Passive.StarterBlade");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Passive_StarterTome, "Item.Passive.StarterTome");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Passive_GuardianGrail, "Item.Passive.GuardianGrail");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Passive_HeroAxe, "Item.Passive.HeroAxe");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Passive_ArmorPen_Minor, "Item.Passive.ArmorPen.Minor");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Passive_ArmorPen_Major, "Item.Passive.ArmorPen.Major");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Passive_MagicPen_Minor, "Item.Passive.MagicPen.Minor");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Passive_MagicPen_Major, "Item.Passive.MagicPen.Major");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Passive_Berserker, "Item.Passive.Berserker");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Passive_PlatedBoots, "Item.Passive.PlatedBoots");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Passive_Bloodthirster, "Item.Passive.Bloodthirster");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Passive_Executioner, "Item.Passive.Executioner");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Passive_BlackCleaver, "Item.Passive.BlackCleaver");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Passive_FrostOrb, "Item.Passive.FrostOrb");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Passive_ThornMail, "Item.Passive.ThornMail");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Passive_GreatNature, "Item.Passive.GreatNature");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Passive_ArcaneSurge, "Item.Passive.ArcaneSurge");
UE_DEFINE_GAMEPLAY_TAG(TAG_Cooldown_Item_ArcaneSurge, "Item.ArcaneSurge.Cooldown");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Passive_NatureGuard, "Item.Passive.NatureGuard");
UE_DEFINE_GAMEPLAY_TAG(TAG_Item_Passive_FrostWarden, "Item.Passive.FrostWarden");

UE_DEFINE_GAMEPLAY_TAG(TAG_ItemCategory_Starter, "ItemCategory.Starter");

// 체력 구간 태그 (서버에서만 LooseTag로 붙임)
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Health_Below30, "Status.Health.Below30");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Health_Below40, "Status.Health.Below40");
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Health_Below50, "Status.Health.Below50");

void UCItemStatics::UpdateHealthThresholdTags(UAbilitySystemComponent* ASC, float Health, float MaxHealth)
{
	if ( !ASC || !ASC->GetOwner() || !ASC->GetOwner()->HasAuthority() )
		return;

	const float Ratio = (Health > 0.f && MaxHealth > 0.f) ? Health / MaxHealth : 1.f;

	const bool bReplicate = UCAbilitySystemStatics::IsHero(ASC->GetAvatarActor());

	SetLooseTagState(ASC, TAG_Status_Health_Below30, Ratio <= 0.3f, bReplicate);
	SetLooseTagState(ASC, TAG_Status_Health_Below40, Ratio <= 0.4f, bReplicate);
	SetLooseTagState(ASC, TAG_Status_Health_Below50, Ratio <= 0.5f, bReplicate);
}

float UCItemStatics::GetDefensePenetrationMultiplier(const FGameplayTagContainer* SourceTags, bool bIsMagical)
{
	const UCGameplayEffectSettings* Settings = GetDefault<UCGameplayEffectSettings>();
	if ( !SourceTags || !Settings )
		return 1.f;

	// 관통력은 큰 수치 하나만 적용
	float Penetration = 0.f;

	if ( bIsMagical )
	{
		// 균열의 보석
		if ( SourceTags->HasTagExact(TAG_Item_Passive_MagicPen_Minor) )
			Penetration = FMath::Max(Penetration, Settings->MagicPenetrationMinor);
		
		// 공허의 지팡이
		if ( SourceTags->HasTagExact(TAG_Item_Passive_MagicPen_Major) )
			Penetration = FMath::Max(Penetration, Settings->MagicPenetrationMajor);
	}
	else
	{
		// 갑옷 분쇄기
		if ( SourceTags->HasTagExact(TAG_Item_Passive_ArmorPen_Minor) )
			Penetration = FMath::Max(Penetration, Settings->ArmorPenetrationMinor);

		// 파멸의 창
		if ( SourceTags->HasTagExact(TAG_Item_Passive_ArmorPen_Major) )
			Penetration = FMath::Max(Penetration, Settings->ArmorPenetrationMajor);
	}

	return 1.f - FMath::Clamp(Penetration, 0.f, 1.f);
}

float UCItemStatics::GetDamageMultiplier(const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, bool bIsBasicAttack)
{
	const UCGameplayEffectSettings* Settings = GetDefault<UCGameplayEffectSettings>();
	if ( !Settings )
		return 1.f;

	float Multiplier = 1.f;

	// 처형자의 대낫
	if ( SourceTags && TargetTags && SourceTags->HasTagExact(TAG_Item_Passive_Executioner) && TargetTags->HasTagExact(TAG_Status_Health_Below50) )
	{
		Multiplier *= 1.f + Settings->ExecutionerDamageAmp;
	}

	// 판금 장화
	if( bIsBasicAttack && TargetTags && TargetTags->HasTagExact(TAG_Item_Passive_PlatedBoots) )
	{
		Multiplier *= 1.f - Settings->PlatedBootsBasicAttackReduction;
	}

	// 서리 군주의 성벽
	if ( bIsBasicAttack && TargetTags && TargetTags->HasTagExact(TAG_Item_Passive_FrostWarden) )
	{
		Multiplier *= 1.f - Settings->FrostWardenBasicAttackReduction;
	}

	return Multiplier;
}

void UCItemStatics::HandleDamageDealt(const FGameplayEffectModCallbackData& Data, UAbilitySystemComponent* VictimASC, float DamageDealt)
{
	const UCGameplayEffectSettings* Settings = GetDefault<UCGameplayEffectSettings>();
	if ( !Settings || !VictimASC || DamageDealt <= 0.f )
		return;

	AActor* VictimActor = VictimASC->GetAvatarActor();
	if ( !VictimActor || !VictimActor->HasAuthority() )
		return;

	const FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	UAbilitySystemComponent* SourceASC = Context.GetInstigatorAbilitySystemComponent();
	AActor* Causer = Context.GetEffectCauser();

	if ( !SourceASC || SourceASC == VictimASC )
		return;

	FGameplayTagContainer EffectAssetTags;
	Data.EffectSpec.GetAllAssetTags(EffectAssetTags);

	if ( EffectAssetTags.HasTag(UCAbilitySystemStatics::GetItemBonusDamageTag()) )
		return;

	const bool bIsMagical = EffectAssetTags.HasTag(UCAbilitySystemStatics::GetMagicalDamageTypeTag());
	const bool bIsBasicAttack = EffectAssetTags.HasTag(UCAbilitySystemStatics::GetBasicAttackTypeTag());
	const bool bVictimIsStructure = IsStructure(VictimActor);

	// 흡혈 : 기본 공격 물리 피해 * LifeStealRatio
	if ( bIsBasicAttack && !bIsMagical && !bVictimIsStructure && !UCAbilitySystemStatics::IsActorDead(SourceASC->GetAvatarActor()) )
	{
		bool bFound = false;
		float LifeStealRatio = SourceASC->GetGameplayAttributeValue(UCHeroAttributeSet::GetLifeStealRatioAttribute(), bFound);

		if ( bFound && LifeStealRatio > 0.f )
		{
			const float LifeStealAmount = DamageDealt * LifeStealRatio;

			if ( FPendingLifeSteal* Pending = PendingLifeSteal.Find(SourceASC) )
			{
				Pending->Sum += LifeStealAmount;
				Pending->Max = FMath::Max(Pending->Max, LifeStealAmount);
			}
			else
			{
				ApplyItemHeal(SourceASC, SourceASC, LifeStealAmount, true);
			}
		}
	}

	// 기본 공격 시 적중 효과
	if ( bIsBasicAttack && !bVictimIsStructure )
	{
		// 용사의 도끼
		if ( SourceASC->HasMatchingGameplayTag(TAG_Item_Passive_HeroAxe) )
		{
			ApplyItemEffect(SourceASC, SourceASC, Settings->HeroAxeHasteEffect);
		}

		// 광전사의 신발
		if ( SourceASC->HasMatchingGameplayTag(TAG_Item_Passive_Berserker) )
		{
			ApplyItemEffect(SourceASC, SourceASC, Settings->BerserkerHasteEffect);
		}
	}

	// 피해 적중 시 효과
	if ( !bVictimIsStructure )
	{
		// 흑색 도끼
		if ( !bIsMagical && SourceASC->HasMatchingGameplayTag(TAG_Item_Passive_BlackCleaver) )
		{
			ApplyItemEffect(SourceASC, VictimASC, Settings->BlackCleaverShredEffect);
		}

		// 서리 수정구
		if ( bIsMagical && SourceASC->HasMatchingGameplayTag(TAG_Item_Passive_FrostOrb) )
		{
			ApplyItemEffect(SourceASC, VictimASC, Settings->FrostOrbSlowEffect);
		}

		if ( !bIsBasicAttack && SourceASC->HasMatchingGameplayTag(TAG_Item_Passive_ArcaneSurge) && !SourceASC->HasMatchingGameplayTag(TAG_Cooldown_Item_ArcaneSurge) && !UCAbilitySystemStatics::IsActorDead(VictimActor) )
		{
			bool bFound = false;

			const float Intelligence = SourceASC->GetGameplayAttributeValue(UCHeroAttributeSet::GetIntelligenceAttribute(), bFound);

			ApplyItemEffect(SourceASC, SourceASC, Settings->ArcaneSurgeCooldownEffect);
			ApplyItemMagicDamage(SourceASC, VictimASC, Settings->ArcaneSurgeBaseDamage + Intelligence * Settings->ArcaneSurgeIntelligenceRatio);
		}
	}

	// 기본 공격 피격 시 가시갑옷 적용
	if ( bIsBasicAttack && VictimASC->HasMatchingGameplayTag(TAG_Item_Passive_ThornMail) && !UCAbilitySystemStatics::IsActorDead(VictimActor) )
	{
		UAbilitySystemComponent* AttackerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Causer);
		if ( AttackerASC && AttackerASC != VictimASC && !IsStructure(Causer) && !UCAbilitySystemStatics::IsActorDead(Causer) )
		{
			bool bFound = false;
			const float VictimArmor = VictimASC->GetGameplayAttributeValue(UCAttributeSet::GetArmorAttribute(), bFound);

			ApplyItemMagicDamage(VictimASC, AttackerASC, Settings->ThornmailBaseDamage + VictimArmor * Settings->ThornmailArmorRatio);
		}
	}

	if ( bIsMagical && VictimASC && VictimASC->HasMatchingGameplayTag(TAG_Item_Passive_NatureGuard) && !UCAbilitySystemStatics::IsActorDead(VictimActor) ) 
	{
		ApplyItemEffect(VictimASC, VictimASC, Settings->NatureGuardStackEffect);
	}
}

void UCItemStatics::HandleAllySupported(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC)
{
	const UCGameplayEffectSettings* Settings = GetDefault<UCGameplayEffectSettings>();
	if ( !Settings || !SourceASC || !TargetASC || SourceASC == TargetASC )
		return;

	// 대자연의 지팡이
	if ( SourceASC->HasMatchingGameplayTag(TAG_Item_Passive_GreatNature) )
	{
		AActor* SourceActor = SourceASC->GetAvatarActor();
		AActor* TargetActor = TargetASC->GetAvatarActor();

		if ( SourceActor && TargetActor && SourceActor->HasAuthority() && UCAbilitySystemStatics::IsHero(TargetActor))
		{
			const IGenericTeamAgentInterface* SourceTeamInterface = Cast<IGenericTeamAgentInterface>(SourceActor);
			if ( SourceTeamInterface && SourceTeamInterface->GetTeamAttitudeTowards(*TargetActor) == ETeamAttitude::Friendly )
			{
				ApplyItemEffect(SourceASC, TargetASC, Settings->GreatNatureBuffEffect);
				ApplyItemEffect(SourceASC, SourceASC, Settings->GreatNatureBuffEffect);
			}
		}
	}
}

bool UCItemStatics::IsAbilityCostEffect(const FGameplayEffectSpec& Spec)
{
	const UGameplayAbility* Ability = Spec.GetContext().GetAbility();

	return Ability && Spec.Def && Ability->GetCostGameplayEffect() == Spec.Def.Get();
}

float UCItemStatics::GetManaRefund(const UAbilitySystemComponent* ASC, float ManaSpent)
{
	const UCGameplayEffectSettings* Settings = GetDefault<UCGameplayEffectSettings>();
	if ( !Settings || !ASC || ManaSpent <= 0.f)
		return 0.f;

	return ASC->HasMatchingGameplayTag(TAG_Item_Passive_StarterTome) ? ManaSpent * Settings->StarterTomeManaRefundRatio : 0.f;
}

float UCItemStatics::GetBonusKillGold(const AActor* Killer, bool bVictimIsHero)
{
	const UCGameplayEffectSettings* Settings = GetDefault<UCGameplayEffectSettings>();
	if ( !Settings || !Killer || bVictimIsHero )
		return 0.f;

	return UCAbilitySystemStatics::ActorHasTag(Killer, TAG_Item_Passive_StarterBlade) ? Settings->StarterBladeBonusGold : 0.f;
}

float UCItemStatics::GetBonusNonKillerGold(const AActor* Receiver)
{
	const UCGameplayEffectSettings* Settings = GetDefault<UCGameplayEffectSettings>();
	if ( !Settings || !Receiver )
		return 0.f;

	return UCAbilitySystemStatics::ActorHasTag(Receiver, TAG_Item_Passive_GuardianGrail) ? Settings->GuardianGrailBonusGold : 0.f;
}

void UCItemStatics::HandleHeroKill(AActor* Killer)
{
	const UCGameplayEffectSettings* Settings = GetDefault<UCGameplayEffectSettings>();
	if ( !Settings || !UCAbilitySystemStatics::ActorHasTag(Killer, TAG_Item_Passive_Bloodthirster) )
		return;

	UAbilitySystemComponent* KillerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Killer);
	if ( !KillerASC )
		return;

	bool bFound = false;
	float MaxHealth = KillerASC->GetGameplayAttributeValue(UCAttributeSet::GetMaxHealthAttribute(), bFound);

	ApplyItemHeal(KillerASC, KillerASC, MaxHealth * Settings->BloodthirsterKillHealRatio, false);
}

void UCItemStatics::ApplyItemMagicDamage(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float BaseDamage)
{
	const UCGameplayEffectSettings* Settings = GetDefault<UCGameplayEffectSettings>();
	TSubclassOf<UGameplayEffect> DamageEffect = Settings ? LoadItemEffect(Settings->ItemMagicDamageEffect) : nullptr;

	if ( !DamageEffect || !SourceASC || !TargetASC || BaseDamage <= 0.f )
		return;

	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffect, 1.f, SourceASC->MakeEffectContext());
	if ( !Spec.IsValid() )
		return;

	Spec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetAbilityBaseDamageTag(), BaseDamage);
	Spec.Data->AddDynamicAssetTag(UCAbilitySystemStatics::GetItemBonusDamageTag());

	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
}

void UCItemStatics::ApplyItemHeal(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float Amount, bool bIsLifeSteal)
{
	const UCGameplayEffectSettings* Settings = GetDefault<UCGameplayEffectSettings>();
	TSubclassOf<UGameplayEffect> HealEffect = Settings ? LoadItemEffect(bIsLifeSteal ? Settings->LifeStealHealEffect : Settings->ItemHealEffect) : nullptr;
	
	if ( !HealEffect || !SourceASC || !TargetASC || Amount <= 0.f )
		return;

	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(HealEffect, 1.f, SourceASC->MakeEffectContext());
	if ( !Spec.IsValid() )
		return;

	Spec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetHealMagnitudeTag(), Amount);

	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
}

void UCItemStatics::ApplyItemEffect(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, const TSoftClassPtr<UGameplayEffect>& Effect)
{
	TSubclassOf<UGameplayEffect> EffectClass = LoadItemEffect(Effect);
	if ( !EffectClass || !SourceASC || !TargetASC )
		return;

	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(EffectClass, 1.f, SourceASC->MakeEffectContext());
	if ( Spec.IsValid() )
	{
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}

void UCItemStatics::BeginLifeStealBatch(UAbilitySystemComponent* SourceASC)
{
	if ( !SourceASC )
		return;

	PendingLifeSteal.FindOrAdd(SourceASC);
}

void UCItemStatics::EndLifeStealBatch(UAbilitySystemComponent* SourceASC)
{
	if ( !SourceASC )
		return;

	FPendingLifeSteal Pending;
	if ( !PendingLifeSteal.RemoveAndCopyValue(SourceASC, Pending) && Pending.Sum<= 0.f )
		return;

	const UCGameplayEffectSettings* Settings = GetDefault<UCGameplayEffectSettings>();
	const float MultiHitRatio = Settings ? Settings->MultiHitLifeStealRatio : 0.5f;

	const float TotalLifeSteal = Pending.Max + (Pending.Sum - Pending.Max) * MultiHitRatio;

	ApplyItemHeal(SourceASC, SourceASC, TotalLifeSteal, true);
}

TSubclassOf<UGameplayEffect> UCItemStatics::LoadItemEffect(const TSoftClassPtr<UGameplayEffect>& Effect)
{
	return Effect.IsNull() ? nullptr : Effect.LoadSynchronous();
}

void UCItemStatics::SetLooseTagState(UAbilitySystemComponent* ASC, const FGameplayTag& Tag, bool bShouldHave, bool bReplicate)
{
	const bool bHas = ASC->GetTagCount(Tag) > 0;

	if ( bShouldHave && !bHas )
	{
		ASC->AddLooseGameplayTag(Tag);

		if ( bReplicate )
		{
			ASC->AddReplicatedLooseGameplayTag(Tag);
		}
	}
	else if ( !bShouldHave && bHas )
	{
		ASC->RemoveLooseGameplayTag(Tag);

		if ( bReplicate )
		{
			ASC->RemoveReplicatedLooseGameplayTag(Tag);
		}
	}
}

bool UCItemStatics::IsStructure(const AActor* Actor)
{
	return Actor && Actor->IsA<ATurret>();
}