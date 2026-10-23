// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/CAttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AI/AggroSubsystem.h"
#include "AI/Turret/Turret.h"
#include "Character/CCharacter.h"
#include "GameplayEffectExtension.h"
#include "GAS/CGameplayEffectSettings.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/Items/CItemStatics.h"
#include "Net/UnrealNetwork.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Framework/CoreBench.h"

void UCAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	if (Attribute == GetHealthAttribute())
	{
		PreDamageHealth = GetHealth();

		if ( NewValue < PreDamageHealth )
		{
			bool bIsInvincible = false;

			if ( UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent() )
			{
				bIsInvincible = ASC->HasMatchingGameplayTag(UCAbilitySystemStatics::GetInvincibleStatusTag());
			}

			if ( bIsInvincible )
			{
				NewValue = PreDamageHealth;
				RawIntendedDamage = 0.f;
				RawIntendedHeal = 0.f;
			}

			else
			{
				float IncomingDamage = PreDamageHealth - NewValue;
				float CurrentShield = GetShield();

				if ( CurrentShield > 0.f )
				{
					float AbsorbedByShield = FMath::Min(CurrentShield, IncomingDamage);
					float NewShieldValue = CurrentShield - AbsorbedByShield;
					SetShield(CurrentShield - AbsorbedByShield);

					if ( NewShieldValue <= 0.f ) 
					{
						if ( ACCharacter* OwnerCharacter = Cast<ACCharacter>(GetOwningActor()) )
						{
							OwnerCharacter->ClearShieldBuffEarly();
						}
					}

					float RemainingDamage = IncomingDamage - AbsorbedByShield;
					NewValue = PreDamageHealth - RemainingDamage;
				}

				RawIntendedDamage = IncomingDamage;
				RawIntendedHeal = 0.f;
			}
		}
		else if ( NewValue > PreDamageHealth )
		{
			RawIntendedHeal = NewValue - PreDamageHealth;
			RawIntendedDamage = 0.f;
		}
		else
		{
			RawIntendedHeal = 0.f;
			RawIntendedDamage = 0.f;
		}

		NewValue = FMath::Clamp(NewValue, 0, GetMaxHealth());
	}

	if ( Attribute == GetShieldAttribute() )
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}

	if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0, GetMaxMana());
	}
}

void UCAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Core_AttributePostGameplayEffectExecute);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		float ClampedHealth = FMath::Clamp(GetHealth(), 0.f, GetMaxHealth());
		float ActualDamage = FMath::Max(0.f, PreDamageHealth - ClampedHealth);
		float ActualHeal = FMath::Max(0.f, ClampedHealth - PreDamageHealth);
		float CapturedRawIntendedDamage = RawIntendedDamage;
		float CapturedRawIntendedHeal = RawIntendedHeal;

		SetHealth(ClampedHealth);
		SetCachedHealthPercent(GetHealth() / GetMaxHealth());

		if (ActualDamage > 0.f)
		{
			float DisplayDamage = FMath::Max(ActualDamage, CapturedRawIntendedDamage);

			if ( UAbilitySystemComponent* TargetASC = GetOwningAbilitySystemComponent() )
			{
				FGameplayEventData DamagedEventData;
				DamagedEventData.EventMagnitude = ActualDamage;
				TargetASC->HandleGameplayEvent(UCAbilitySystemStatics::GetDamagedEventTag(), &DamagedEventData);
			}

			AActor* Causer = Data.EffectSpec.GetContext().GetEffectCauser();

			if ( Causer && Causer != GetOwningActor() && UCAbilitySystemStatics::IsHero(Causer) )
			{
				if ( ACCharacter* VictimCharacter = Cast<ACCharacter>(GetOwningActor()) )
				{
					VictimCharacter->RecordDamagingHero(Causer);
				}
				else if ( ATurret* VictimTurret = Cast<ATurret>(GetOwningActor()) )
				{
					VictimTurret->RecordDamagingHero(Causer);
				}
			}

			if ( Causer && Causer != GetOwningActor() )
			{
				if ( UAggroSubsystem* AggroSubsystem = GetWorld()->GetSubsystem<UAggroSubsystem>() )
				{
					AggroSubsystem->BroadcastThreat(GetOwningActor(), Causer);
				}
			}

			static const bool bNoOptPopupFilter = CoreBench::IsOptOff(TEXT("NoOptPopupFilter"));

			const bool bHeroInvolvedDamged = bNoOptPopupFilter
				|| UCAbilitySystemStatics::IsHero(GetOwningActor())
				|| UCAbilitySystemStatics::IsHero(Causer)
				|| UCAbilitySystemStatics::IsHero(Data.EffectSpec.GetContext().GetInstigator());

			UAbilitySystemComponent* TargetASC = bHeroInvolvedDamged ? GetOwningAbilitySystemComponent() : nullptr;
			if ( TargetASC )
			{
				FGameplayCueParameters DamageNumberParams;

				FVector PopupBaseLocation;
				if ( const FHitResult* HitResultPtr = Data.EffectSpec.GetContext().GetHitResult() )
				{
					PopupBaseLocation = HitResultPtr->ImpactPoint;
				}
				else
				{
					PopupBaseLocation = GetOwningActor()->GetTargetLocation() + FVector(0.f, 0.f, 100.f);
				}

				/*FVector PopupDamageOffset(
					-20.f,
					FMath::FRandRange(-30.f, 30.f),
					FMath::FRandRange(0.f, 25.f)
				);*/

				FGameplayTagContainer EffectAssetTags;
				Data.EffectSpec.GetAllAssetTags(EffectAssetTags);

				DamageNumberParams.RawMagnitude = DisplayDamage;
				DamageNumberParams.Location = PopupBaseLocation; //+ PopupDamageOffset;
				DamageNumberParams.EffectContext = Data.EffectSpec.GetContext();
				DamageNumberParams.AggregatedSourceTags = EffectAssetTags;
				DamageNumberParams.Instigator = Data.EffectSpec.GetContext().GetInstigator();

				TargetASC->ExecuteGameplayCue(UCAbilitySystemStatics::GetDamagePopupGameplayCueTag(), DamageNumberParams);
			}

			UAbilitySystemComponent* SourceASC = Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent();

			const UCGameplayEffectSettings* EffectSettings = GetDefault<UCGameplayEffectSettings>();
			TSubclassOf<UGameplayEffect> GiantSlayerEffect = EffectSettings ? EffectSettings->GiantSlayerBonusDamageEffect.LoadSynchronous() : nullptr;

			if ( Causer && GiantSlayerEffect && Data.EffectSpec.Def && Data.EffectSpec.Def->GetAssetTags().HasTag(UCAbilitySystemStatics::GetBasicAttackTypeTag()) )
			{
				if ( SourceASC && SourceASC->HasMatchingGameplayTag(UCAbilitySystemStatics::GetGiantSlayerEquippedTag()) )
				{
					AActor* VictimActor = GetOwningActor();

					if ( VictimActor && VictimActor != Causer && UCAbilitySystemStatics::CanApplyGiantSlayerTo(VictimActor) )
					{
						UAbilitySystemComponent* VictimASC = GetOwningAbilitySystemComponent();

						if ( VictimASC )
						{
							FGameplayEffectSpecHandle BonusSpec = SourceASC->MakeOutgoingSpec(GiantSlayerEffect, 1, SourceASC->MakeEffectContext());
							if ( BonusSpec.IsValid() )
							{
								BonusSpec.Data->SetSetByCallerMagnitude(UCAbilitySystemStatics::GetMaxHealthPercentDataTag(), UCAbilitySystemStatics::GiantSlayerMaxHealthRatio);
								BonusSpec.Data->AddDynamicAssetTag(UCAbilitySystemStatics::GetItemBonusDamageTag()); 
								SourceASC->ApplyGameplayEffectSpecToTarget(*BonusSpec.Data.Get(), VictimASC);
							}
						}
					}
				}
			}

			UCItemStatics::HandleDamageDealt(Data, GetOwningAbilitySystemComponent(), DisplayDamage);
		}

		const float AttemptedHeal = (Data.EvaluatedData.ModifierOp == EGameplayModOp::Additive && Data.EvaluatedData.Magnitude > 0.f) ? Data.EvaluatedData.Magnitude : 0.f;

		if ( ActualHeal > 0.f || AttemptedHeal > 0.f )
		{
			float DisplayHeal = FMath::Max3(ActualHeal, CapturedRawIntendedHeal, AttemptedHeal);

			FGameplayTagContainer EffectAssetTags;
			if ( Data.EffectSpec.Def )
			{
				EffectAssetTags = Data.EffectSpec.Def->GetAssetTags();
			}

			const bool bIsRegen = EffectAssetTags.HasTag(UCAbilitySystemStatics::GetRegenHealTypeTag());

			// 흡혈은 실제 회복량이 있을 때만 출력
			const bool bIsWastedLifeSteal = ActualHeal <= 0.f && EffectAssetTags.HasTag(UCAbilitySystemStatics::GetLifeStealHealTypeTag());

			if ( !bIsRegen && !bIsWastedLifeSteal && UCAbilitySystemStatics::IsHero(GetOwningActor()) )
			{
				UCItemStatics::HandleAllySupported(Data.EffectSpec.GetContext().GetInstigatorAbilitySystemComponent(), GetOwningAbilitySystemComponent());

				if ( UAbilitySystemComponent* TargetASC = GetOwningAbilitySystemComponent() )
				{
					FGameplayCueParameters HealPopupParams;
					HealPopupParams.RawMagnitude = DisplayHeal;

					HealPopupParams.Location = GetOwningActor()->GetActorLocation() + FVector(-25.f, 0.f, 50.f) + FVector(0, FMath::FRandRange(-25.f, 25.f), 0.f);

					HealPopupParams.EffectContext = Data.EffectSpec.GetContext();
					HealPopupParams.AggregatedSourceTags = EffectAssetTags;
					HealPopupParams.Instigator = Data.EffectSpec.GetContext().GetInstigator();

					TargetASC->ExecuteGameplayCue(UCAbilitySystemStatics::GetHealPopupGameplayCueTag(), HealPopupParams);
				}
			}
		}
	}

	if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		if ( Data.EvaluatedData.Magnitude < 0.f && UCItemStatics::IsAbilityCostEffect(Data.EffectSpec) )
		{
			SetMana(GetMana() + UCItemStatics::GetManaRefund(GetOwningAbilitySystemComponent(), -Data.EvaluatedData.Magnitude));
		}

		SetMana(FMath::Clamp(GetMana(), 0, GetMaxMana()));
		SetCachedManaPercent(GetMana() / GetMaxMana());
	}
}

bool UCAttributeSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	if ( !Super::PreGameplayEffectExecute(Data) )
		return false;

	if ( Data.EvaluatedData.Attribute == GetHealthAttribute() && Data.EvaluatedData.Magnitude < 0.f )
	{
		UAbilitySystemComponent* OwnerASC = GetOwningAbilitySystemComponent();

		if ( OwnerASC && OwnerASC->HasMatchingGameplayTag(UCAbilitySystemStatics::GetOnlyBasicAttackDamageTag()) )
		{
			bool bIsBasicAttack = Data.EffectSpec.Def->GetAssetTags().HasTag(UCAbilitySystemStatics::GetBasicAttackTypeTag());

			if ( !bIsBasicAttack )
				return false;
		}
	}

	return true;
}

void UCAttributeSet::RescaleHealth()
{
	if (!GetOwningActor()->HasAuthority())
		return;

	if (GetCachedHealthPercent() != 0 && GetHealth() != 0)
	{
		SetHealth(GetMaxHealth() * GetCachedHealthPercent());
	}
}

void UCAttributeSet::RescaleMana()
{
	if (!GetOwningActor()->HasAuthority())
		return;

	if (GetCachedManaPercent() != 0 && GetMana() != 0)
	{
		SetMana(GetMaxMana() * GetCachedManaPercent());
	}
}

void UCAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCAttributeSet, Health, OldValue);
}

void UCAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCAttributeSet, MaxHealth, OldValue);
}

void UCAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCAttributeSet, Mana, OldValue);
}

void UCAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCAttributeSet, MaxMana, OldValue);
}

void UCAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCAttributeSet, Armor, OldValue);
}

void UCAttributeSet::OnRep_MagicResistance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCAttributeSet, MagicResistance, OldValue);
}

void UCAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCAttributeSet, MoveSpeed, OldValue);
}

void UCAttributeSet::OnRep_MoveAcceleration(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCAttributeSet, MoveAcceleration, OldValue);
}

void UCAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCAttributeSet, Shield, OldValue);
}

void UCAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UCAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCAttributeSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCAttributeSet, MagicResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCAttributeSet, MoveAcceleration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCAttributeSet, Shield, COND_None, REPNOTIFY_Always);
}