// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/CAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Character/CCharacter.h"
#include "Framework/CGameState.h"
#include "GameplayEffectExtension.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/CGameplayAbilityTypes.h"
#include "GAS/CGameplayEffectSettings.h"
#include "GAS/CAttributeSet.h"
#include "GAS/CHeroAttributeSet.h"
#include "GAS/Items/CItemStatics.h"
#include "Player/CPlayerState.h"

UCAbilitySystemComponent::UCAbilitySystemComponent()
{
	GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetHealthAttribute()).AddUObject(this, &UCAbilitySystemComponent::HealthUpdated);
	GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetManaAttribute()).AddUObject(this, &UCAbilitySystemComponent::ManaUpdated);
	GetGameplayAttributeValueChangeDelegate(UCHeroAttributeSet::GetExperienceAttribute()).AddUObject(this, &UCAbilitySystemComponent::ExperienceUpdated);

	GenericConfirmInputID = (int32)ECAbilityInputID::Confirm;
	GenericCancelInputID = (int32)ECAbilityInputID::Cancel;
}

void UCAbilitySystemComponent::InitializeBaseAttributes()
{
	if (!AbilitySystemGenerics || !AbilitySystemGenerics->GetBaseStatDataTable() || !GetOwner())
		return;

	const UDataTable* BaseStatDataTable = AbilitySystemGenerics->GetBaseStatDataTable();

	const FHeroBaseStats* BaseStats = nullptr;

	for ( const TPair<FName, uint8*>& DataPair : BaseStatDataTable->GetRowMap() )
	{
		const FHeroBaseStats* Row = BaseStatDataTable->FindRow<FHeroBaseStats>(DataPair.Key, "");
		if ( Row && Row->Class == GetOwner()->GetClass() )
		{
			BaseStats = Row;
			break;
		}
	}

	if ( !BaseStats )
	{
		UE_LOG(LogTemp, Error, TEXT("[BaseStats] %s: BaseStat DataTable에 Class가 일치하는 행이 없습니다. 스탯이 0으로 시작합니다."), *GetNameSafe(GetOwner()->GetClass()));
	}

	if (BaseStats)
	{
		CachedBaseStats = *BaseStats;
		bHasCachedBaseStats = true;

		SetNumericAttributeBase(UCAttributeSet::GetMaxHealthAttribute(), BaseStats->BaseMaxHealth);
		SetNumericAttributeBase(UCAttributeSet::GetMaxManaAttribute(), BaseStats->BaseMaxMana);
		SetNumericAttributeBase(UCAttributeSet::GetArmorAttribute(), BaseStats->BaseArmor);
		SetNumericAttributeBase(UCAttributeSet::GetMagicResistanceAttribute(), BaseStats->BaseMagicResistance);
		SetNumericAttributeBase(UCAttributeSet::GetMoveSpeedAttribute(), BaseStats->BaseMoveSpeed);

		SetNumericAttributeBase(UCHeroAttributeSet::GetStrengthAttribute(), BaseStats->Strength);
		SetNumericAttributeBase(UCHeroAttributeSet::GetStrengthGrowthRateAttribute(), BaseStats->StrengthGrowthRate);
		SetNumericAttributeBase(UCHeroAttributeSet::GetIntelligenceAttribute(), BaseStats->Intelligence);
		SetNumericAttributeBase(UCHeroAttributeSet::GetIntelligenceGrowthRateAttribute(), BaseStats->IntelligenceGrowthRate);
		SetNumericAttributeBase(UCHeroAttributeSet::GetLifeStealRatioAttribute(), 0.f);

		SetNumericAttributeBase(UCHeroAttributeSet::GetMaxHealthGrowthRateAttribute(), BaseStats->MaxHealthGrowthRate);
		SetNumericAttributeBase(UCHeroAttributeSet::GetMaxManaGrowthRateAttribute(), BaseStats->MaxManaGrowthRate);
		SetNumericAttributeBase(UCHeroAttributeSet::GetArmorGrowthRateAttribute(), BaseStats->ArmorGrowthRate);
		SetNumericAttributeBase(UCHeroAttributeSet::GetMagicResistanceGrowthRateAttribute(), BaseStats->MagicResistanceGrowthRate);

		SetNumericAttributeBase(UCHeroAttributeSet::GetHealthRegenAttribute(), BaseStats->BaseHealthRegen);
		SetNumericAttributeBase(UCHeroAttributeSet::GetHealthRegenGrowthRateAttribute(), BaseStats->HealthRegenGrowthRate);
		SetNumericAttributeBase(UCHeroAttributeSet::GetManaRegenAttribute(), BaseStats->BaseManaRegen);
		SetNumericAttributeBase(UCHeroAttributeSet::GetManaRegenGrowthRateAttribute(), BaseStats->ManaRegenGrowthRate);

		SetNumericAttributeBase(UCAttributeSet::GetMoveAccelerationAttribute(), BaseStats->BaseMoveAcceleration);
	}

	const FRealCurve* ExperienceCurve = AbilitySystemGenerics->GetExperienceCurve();
	if (ExperienceCurve)
	{
		int MaxLevel = ExperienceCurve->GetNumKeys();
		SetNumericAttributeBase(UCHeroAttributeSet::GetMaxLevelAttribute(), MaxLevel);

		float MaxExp = ExperienceCurve->GetKeyValue(ExperienceCurve->GetLastKeyHandle());
		SetNumericAttributeBase(UCHeroAttributeSet::GetMaxLevelExperienceAttribute(), MaxExp);

		UE_LOG(LogTemp, Warning, TEXT("Max Level is %d, Max Exp if %f"), MaxLevel, MaxExp);
	}

	ExperienceUpdated(FOnAttributeChangeData());
}

void UCAbilitySystemComponent::ServerSideInit()
{
	InitializeBaseAttributes();
	ApplyInitialEffects();
	GiveInitialAbilities();
}

void UCAbilitySystemComponent::ApplyInitialEffects()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return;

	if (!AbilitySystemGenerics)
		return;

	for (const TSubclassOf<UGameplayEffect>& EffectClass : AbilitySystemGenerics->GetInitialEffects())
	{
		FGameplayEffectSpecHandle EffectSpecHandle = MakeOutgoingSpec(EffectClass, 1, MakeEffectContext());
		ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
	}
}

void UCAbilitySystemComponent::GiveInitialAbilities()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return;

	for (const TPair<ECAbilityInputID, TSubclassOf<UGameplayAbility>>& AbilityPair : Abilities)
	{
		GiveAbility(FGameplayAbilitySpec(AbilityPair.Value, 0, (int32)AbilityPair.Key, nullptr));
	}

	for (const TPair<ECAbilityInputID, TSubclassOf<UGameplayAbility>>& AbilityPair : BasicAbilities)
	{
		GiveAbility(FGameplayAbilitySpec(AbilityPair.Value, 1, (int32)AbilityPair.Key, nullptr));
	}

	if (!AbilitySystemGenerics)
		return;

	for (const TSubclassOf<UGameplayAbility>& PassiveAbility : AbilitySystemGenerics->GetDefaultAbilities())
	{
		GiveAbility(FGameplayAbilitySpec(PassiveAbility, 1, -1, nullptr));
	}
}

void UCAbilitySystemComponent::ApplyFullStatEffect()
{
	if (!AbilitySystemGenerics)
		return;

	AuthApplyGameplayEffect(AbilitySystemGenerics->GetFullStatEffect());
}

void UCAbilitySystemComponent::ApplyMinionTimeScaling(const FMinionScalingConfig& Config, float ElapsedSeconds)
{
	if ( !bHasCachedBaseStats || !GetOwner() || !GetOwner()->HasAuthority() )
		return;

	SetNumericAttributeBase(UCAttributeSet::GetMaxHealthAttribute(), CachedBaseStats.BaseMaxHealth + Config.HealthScaling.GetBonus(ElapsedSeconds));
	SetNumericAttributeBase(UCHeroAttributeSet::GetStrengthAttribute(), CachedBaseStats.Strength + Config.StrengthScaling.GetBonus(ElapsedSeconds));
	SetNumericAttributeBase(UCAttributeSet::GetArmorAttribute(), CachedBaseStats.BaseArmor + Config.ArmorScaling.GetBonus(ElapsedSeconds));
	SetNumericAttributeBase(UCAttributeSet::GetMoveSpeedAttribute(), CachedBaseStats.BaseMoveSpeed + Config.MoveSpeedScaling.GetBonus(ElapsedSeconds));
}

bool UCAbilitySystemComponent::Server_UpgradeAbilityWithID_Validate(ECAbilityInputID InputID)
{
	return true;
}

void UCAbilitySystemComponent::Server_UpgradeAbilityWithID_Implementation(ECAbilityInputID InputID)
{
	bool bFound = false;
	
	float UpgradePoint = GetGameplayAttributeValue(UCHeroAttributeSet::GetUpgradePointAttribute(), bFound);
	if (!bFound || UpgradePoint <= 0)
		return;

	FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromInputID((int32)InputID);
	if (!AbilitySpec || UCAbilitySystemStatics::IsAbilityAtMaxLevel(*AbilitySpec))
		return;

	if ( InputID == ECAbilityInputID::Ultimate )
	{
		float CurrentLevel = GetGameplayAttributeValue(UCHeroAttributeSet::GetLevelAttribute(), bFound);
		int NextUltLevel = AbilitySpec->Level;
		if ( !bFound || CurrentLevel < UCAbilitySystemStatics::UltRequiredLevel[ NextUltLevel ] )
			return;
	}

	SetNumericAttributeBase(UCHeroAttributeSet::GetUpgradePointAttribute(), UpgradePoint - 1);
	AbilitySpec->Level += 1;
	MarkAbilitySpecDirty(*AbilitySpec);

	Client_AbilitySpecLevelUpdated(AbilitySpec->Handle, AbilitySpec->Level);

	if (AbilitySpec->Ability && AbilitySpec->Ability->GetAssetTags().HasTag(FGameplayTag::RequestGameplayTag("Ability.Type.Passive")))
	{
		CancelAbilityHandle(AbilitySpec->Handle);
		TryActivateAbility(AbilitySpec->Handle, false);
	}
}

void UCAbilitySystemComponent::Client_AbilitySpecLevelUpdated_Implementation(FGameplayAbilitySpecHandle Handle, int NewLevel)
{
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
	if (Spec)
	{
		Spec->Level = NewLevel;
		AbilitySpecDirtiedCallbacks.Broadcast(*Spec);
	}
}

bool UCAbilitySystemComponent::IsAtMaxLevel() const
{
	bool bFound;
	float CurrentLevel = GetGameplayAttributeValue(UCHeroAttributeSet::GetLevelAttribute(), bFound);
	float MaxLevel = GetGameplayAttributeValue(UCHeroAttributeSet::GetMaxLevelAttribute(), bFound);

	return CurrentLevel >= MaxLevel;
}

const TMap<ECAbilityInputID, TSubclassOf<UGameplayAbility>>& UCAbilitySystemComponent::GetAbilities() const
{
	return Abilities;
}

void UCAbilitySystemComponent::AuthApplyGameplayEffect(TSubclassOf<UGameplayEffect> GameplayEffect, int Level)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		FGameplayEffectSpecHandle EffectSpecHandle = MakeOutgoingSpec(GameplayEffect, Level, MakeEffectContext());
		ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
	}
}

void UCAbilitySystemComponent::AuthApplyGameplayEffectWithDuration(TSubclassOf<UGameplayEffect> GameplayEffect, float Duration, int Level)
{
	if ( GetOwner() && GetOwner()->HasAuthority() )
	{
		FGameplayEffectSpecHandle EffectSpecHandle = MakeOutgoingSpec(GameplayEffect, Level, MakeEffectContext());
		if ( EffectSpecHandle.IsValid() )
		{
			EffectSpecHandle.Data->SetDuration(Duration, true);
			ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
		}
	}
}

void UCAbilitySystemComponent::HealthUpdated(const FOnAttributeChangeData& ChangeData)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return;

	bool bFound = false;
	float MaxHealth = GetGameplayAttributeValue(UCAttributeSet::GetMaxHealthAttribute(), bFound);

	if (bFound && ChangeData.NewValue >= MaxHealth)
	{
		if (!HasMatchingGameplayTag(UCAbilitySystemStatics::GetHealthFullStatTag()))
		{
			// Replicate 안됨 !! 클라이언트나 서버 둘 중에 필요한 곳에서 호출하는 함수
			AddLooseGameplayTag(UCAbilitySystemStatics::GetHealthFullStatTag());
		}
	}
	else
	{
		RemoveLooseGameplayTag(UCAbilitySystemStatics::GetHealthFullStatTag());
	}

	if (ChangeData.NewValue <= 0)
	{
		if ( !HasMatchingGameplayTag(UCAbilitySystemStatics::GetHealthEmptyStatTag()) )
		{
			AddLooseGameplayTag(UCAbilitySystemStatics::GetHealthEmptyStatTag());

			bool bIsHero = UCAbilitySystemStatics::IsHero(GetOwner());
			ACCharacter* OwnerCharacter = Cast<ACCharacter>(GetOwner());

			if ( bIsHero )
			{
				int32 Level = 1;
				bool bLevelFound = false;
				Level = FMath::Max(1, ( int32 ) GetGameplayAttributeValue(UCHeroAttributeSet::GetLevelAttribute(), bLevelFound));
				float RespawnDelay = OwnerCharacter ? OwnerCharacter->GetRespawnDurationTimeForLevel(Level) : 5.f;

				AuthApplyGameplayEffectWithDuration(AbilitySystemGenerics->GetDeathEffect(), RespawnDelay);

				if ( ACPlayerState* CPlayerState = OwnerCharacter ? OwnerCharacter->GetPlayerState<ACPlayerState>() : nullptr )
				{
					ACGameState* CGameState = GetWorld()->GetGameState<ACGameState>();
					float RespawnAt = (CGameState ? CGameState->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds()) + RespawnDelay;
					CPlayerState->SetRespawnTimeStamp(RespawnAt);
				}
			}
			else
			{
				AuthApplyGameplayEffect(AbilitySystemGenerics->GetDeathEffect());
			}

			FGameplayEventData DeadAbilityEventData;
			if(ChangeData.GEModData)
				DeadAbilityEventData.ContextHandle = ChangeData.GEModData->EffectSpec.GetContext();

			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), UCAbilitySystemStatics::GetDeadStatusTag(), DeadAbilityEventData);
		}	
	}
	else
	{
		RemoveLooseGameplayTag(UCAbilitySystemStatics::GetHealthEmptyStatTag());
	}

	UCItemStatics::UpdateHealthThresholdTags(this, ChangeData.NewValue, MaxHealth);
}

void UCAbilitySystemComponent::ManaUpdated(const FOnAttributeChangeData& ChangeData)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return;

	bool bFound = false;
	float MaxMana = GetGameplayAttributeValue(UCAttributeSet::GetMaxManaAttribute(), bFound);

	if (bFound && ChangeData.NewValue >= MaxMana)
	{
		if (!HasMatchingGameplayTag(UCAbilitySystemStatics::GetManaFullStatTag()))
		{
			// Replicate 안됨 !! 클라이언트에서만 태그 부여
			AddLooseGameplayTag(UCAbilitySystemStatics::GetManaFullStatTag());
		}
	}
	else
	{
		RemoveLooseGameplayTag(UCAbilitySystemStatics::GetManaFullStatTag());
	}

	if (ChangeData.NewValue <= 0)
	{
		if (!HasMatchingGameplayTag(UCAbilitySystemStatics::GetManaEmptyStatTag()))
		{
			AddLooseGameplayTag(UCAbilitySystemStatics::GetManaEmptyStatTag());
		}
	}
	else
	{
		RemoveLooseGameplayTag(UCAbilitySystemStatics::GetManaEmptyStatTag());
	}
}

void UCAbilitySystemComponent::ExperienceUpdated(const FOnAttributeChangeData& ChangeData)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
		return;

	if (IsAtMaxLevel())
		return;

	float CurrentExp = ChangeData.NewValue;

	const FRealCurve* ExperienceCurve = AbilitySystemGenerics->GetExperienceCurve();
	if (!ExperienceCurve)
	{
		UE_LOG(LogTemp, Warning, TEXT("Can't Find Experience Data!"));
		return;
	}

	float PrevLevelExp = 0;
	float NextLevelExp = 0;
	float NewLevel = 1;

	for (auto Iter = ExperienceCurve->GetKeyHandleIterator(); Iter; ++Iter)
	{
		float ExperienceToReachLevel = ExperienceCurve->GetKeyValue(*Iter);
		if (CurrentExp < ExperienceToReachLevel)
		{
			NextLevelExp = ExperienceToReachLevel;
			break;
		}
		PrevLevelExp = ExperienceToReachLevel;

		NewLevel = Iter.GetIndex() + 1;
	}

	float CurrentLevel = GetNumericAttributeBase(UCHeroAttributeSet::GetLevelAttribute());
	float CurrentUpgradePoint = GetNumericAttribute(UCHeroAttributeSet::GetUpgradePointAttribute());

	float LevelUpgraded = NewLevel - CurrentLevel;
	if ( LevelUpgraded > 0 )
	{
		ExecuteGameplayCue(UCAbilitySystemStatics::GetLevelupTag());
	}

	float NewUpgradePoint = CurrentUpgradePoint + LevelUpgraded;

	SetNumericAttributeBase(UCHeroAttributeSet::GetLevelAttribute(), NewLevel);
	SetNumericAttributeBase(UCHeroAttributeSet::GetPrevLevelExperienceAttribute(), PrevLevelExp);
	SetNumericAttributeBase(UCHeroAttributeSet::GetNextLevelExperienceAttribute(), NextLevelExp);
	SetNumericAttributeBase(UCHeroAttributeSet::GetUpgradePointAttribute(), NewUpgradePoint);
}