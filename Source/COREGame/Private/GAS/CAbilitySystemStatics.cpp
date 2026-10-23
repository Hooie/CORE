// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/CAbilitySystemStatics.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemInterface.h"
#include "AI/Turret/Turret.h"
#include "Character/CCharacter.h"
#include "Character/PA_CharacterDefinition.h"
#include "Components/PrimitiveComponent.h"
#include "GameplayCueManager.h"
#include "GAS/CAttributeSet.h"
#include "GAS/CGameplayAbility.h"
#include "GAS/CGameplayAbilityTypes.h"
#include "GAS/Items/CItemStatics.h"
#include "GAS/MMC_AbilityDamage.h"
#include "GenericTeamAgentInterface.h"
#include "NativeGameplayTags.h"
#include "Player/CPlayerState.h"
#include "Player/PlayerInfoTypes.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Status_MatchNotStarted, "Status.MatchNotStarted");

const int32 UCAbilitySystemStatics::UltimateMaxLevel = 3;
const int32 UCAbilitySystemStatics::DefaultAbilityMaxLevel = 5;
const int32 UCAbilitySystemStatics::UltRequiredLevel[ 3 ] = { 6, 11, 16 };

FGameplayTag UCAbilitySystemStatics::GetBasicAttackAbilityTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.BasicAttack");
}

FGameplayTag UCAbilitySystemStatics::GetDeadStatusTag()
{
	return FGameplayTag::RequestGameplayTag("Status.Dead");
}

FGameplayTag UCAbilitySystemStatics::GetStunStatusTag()
{
	return FGameplayTag::RequestGameplayTag("Status.Stun");
}

FGameplayTag UCAbilitySystemStatics::GetDisarmedTag()
{
	return FGameplayTag::RequestGameplayTag("Status.Disarmed");
}

FGameplayTag UCAbilitySystemStatics::GetBindStatusTag()
{
	return FGameplayTag::RequestGameplayTag("Status.Bind");
}

FGameplayTag UCAbilitySystemStatics::GetDisableJumpTag()
{
	return FGameplayTag::RequestGameplayTag("Status.DisableJump");
}

FGameplayTag UCAbilitySystemStatics::GetBasicAttackInputPressedTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.BasicAttack.Pressed");
}

FGameplayTag UCAbilitySystemStatics::GetBasicAttackInputReleasedTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.BasicAttack.Released");
}

FGameplayTag UCAbilitySystemStatics::GetGenericDamageTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Generic.Damage");
}

FGameplayTag UCAbilitySystemStatics::GetAimStatusTag()
{
	return FGameplayTag::RequestGameplayTag("Status.Aim");
}

FGameplayTag UCAbilitySystemStatics::GetFocusStatusTag()
{
	return FGameplayTag::RequestGameplayTag("Status.Focus");
}

FGameplayTag UCAbilitySystemStatics::GetCrosshairTag()
{
	return FGameplayTag::RequestGameplayTag("Status.Crosshair");
}

FGameplayTag UCAbilitySystemStatics::GetCameraShakeGameplayCueTag()
{
	return FGameplayTag::RequestGameplayTag("GameplayCue.CameraShake");
}

FGameplayTag UCAbilitySystemStatics::GetHealthFullStatTag()
{
	return FGameplayTag::RequestGameplayTag("Status.Health.Full");
}

FGameplayTag UCAbilitySystemStatics::GetHealthEmptyStatTag()
{
	return FGameplayTag::RequestGameplayTag("Status.Health.Empty");
}

FGameplayTag UCAbilitySystemStatics::GetManaFullStatTag()
{
	return FGameplayTag::RequestGameplayTag("Status.Mana.Full");
}

FGameplayTag UCAbilitySystemStatics::GetManaEmptyStatTag()
{
	return FGameplayTag::RequestGameplayTag("Status.Mana.Empty");
}

FGameplayTag UCAbilitySystemStatics::GetExperienceAttributeTag()
{
	return FGameplayTag::RequestGameplayTag("Attr.Experience");
}

FGameplayTag UCAbilitySystemStatics::GetGoldAttributeTag()
{
	return FGameplayTag::RequestGameplayTag("Attr.Gold");
}

FGameplayTag UCAbilitySystemStatics::GetLifeStealStatusTag()
{
	return FGameplayTag::RequestGameplayTag("Status.LifeSteal");
}

FGameplayTag UCAbilitySystemStatics::GetLifeStealProcessTag()
{
	return FGameplayTag::RequestGameplayTag("Event.LifeSteal.Process");
}

FGameplayTag UCAbilitySystemStatics::GetHealMagnitudeTag()
{
	return FGameplayTag::RequestGameplayTag("Effect.Magnitude.Heal");
}

FGameplayTag UCAbilitySystemStatics::GetNoWeaponTag()
{
	return FGameplayTag::RequestGameplayTag("Status.NoWeapon");
}

FGameplayTag UCAbilitySystemStatics::GetRecallSwordEventTag()
{
	return FGameplayTag::RequestGameplayTag("Event.Recall.Sword");
}

FGameplayTag UCAbilitySystemStatics::GetActivateEventTag()
{
	return FGameplayTag::RequestGameplayTag("Event.Activate");
}

FGameplayTag UCAbilitySystemStatics::GetLevelupTag()
{
	return FGameplayTag::RequestGameplayTag("GameplayCue.Status.LevelUp");
}

FGameplayTag UCAbilitySystemStatics::GetHeroRoleTag()
{
	return FGameplayTag::RequestGameplayTag("Role.Hero");
}

FGameplayTag UCAbilitySystemStatics::GetTargetUpdatedTag()
{
	return FGameplayTag::RequestGameplayTag("Target.Updated");
}

FGameplayTag UCAbilitySystemStatics::GetPassiveAbilityTypeTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Type.Passive");
}

bool UCAbilitySystemStatics::ActorHasTag(const AActor* ActorToCheck, const FGameplayTag& Tag)
{
	const IAbilitySystemInterface* ActorISA = Cast<IAbilitySystemInterface>(ActorToCheck);
	if (ActorISA)
	{
		UAbilitySystemComponent* ActorASC = ActorISA->GetAbilitySystemComponent();
		if (ActorASC)
		{
			return ActorASC->HasMatchingGameplayTag(Tag);
		}
	}
	return false;
}

bool UCAbilitySystemStatics::IsActorDead(const AActor* ActorToCheck)
{
	return ActorHasTag(ActorToCheck, GetDeadStatusTag());
}

bool UCAbilitySystemStatics::IsHero(const AActor* ActorToCheck)
{
	return ActorHasTag(ActorToCheck, GetHeroRoleTag());
}

bool UCAbilitySystemStatics::IsAbilityAtMaxLevel(const FGameplayAbilitySpec& Spec)
{
	bool bIsUltimate = (Spec.InputID == ( int32 ) ECAbilityInputID::Ultimate);

	int32 MaxLevel = bIsUltimate ? UltimateMaxLevel : DefaultAbilityMaxLevel;

	return Spec.Level >= MaxLevel;
}

float UCAbilitySystemStatics::GetStaticCooldownDurationForAbility(const UGameplayAbility* Ability)
{
	if (!Ability)
		return 0.f;

	const UGameplayEffect* CooldownEffect = Ability->GetCooldownGameplayEffect();
	
	if (!CooldownEffect)
		return 0.f;

	float CooldownDuration = 0.f;

	CooldownEffect->DurationMagnitude.GetStaticMagnitudeIfPossible(1, CooldownDuration);

	return CooldownDuration;
}

float UCAbilitySystemStatics::GetStaticCostForAbility(const UGameplayAbility* Ability)
{
	if (!Ability)
		return 0.f;

	const UGameplayEffect* CostEffect = Ability->GetCostGameplayEffect();

	if (!CostEffect || CostEffect->Modifiers.Num() == 0)
		return 0.f;

	float Cost = 0.f;

	CostEffect->Modifiers[0].ModifierMagnitude.GetStaticMagnitudeIfPossible(1, Cost);

	return FMath::Abs(Cost);
}

bool UCAbilitySystemStatics::CheckCost(const FGameplayAbilitySpec& AbilitySpec, const UAbilitySystemComponent& ASC)
{
	const UGameplayAbility* AbilityCDO = AbilitySpec.Ability;
	if (AbilityCDO)
	{
		return AbilityCDO->CheckCost(AbilitySpec.Handle, ASC.AbilityActorInfo.Get());
	}

	return false;
}

bool UCAbilitySystemStatics::CheckAbilityCostStatic(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC)
{
	if (AbilityCDO)
	{
		return AbilityCDO->CheckCost(FGameplayAbilitySpecHandle(), ASC.AbilityActorInfo.Get());
	}

	return false;
}

float UCAbilitySystemStatics::GetManaCostFor(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC, int AbilityLevel)
{
	float ManaCost = 0.f;

	if (AbilityCDO)
	{
		UGameplayEffect* CostEffect = AbilityCDO->GetCostGameplayEffect();
		if (CostEffect)
		{
			FGameplayEffectSpecHandle EffectSpec = ASC.MakeOutgoingSpec(CostEffect->GetClass(), AbilityLevel, ASC.MakeEffectContext());
			CostEffect->Modifiers[0].ModifierMagnitude.AttemptCalculateMagnitude(*EffectSpec.Data.Get(), ManaCost);
		}
	}

	return FMath::Abs(ManaCost);
}

float UCAbilitySystemStatics::GetCooldownDurationFor(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC, int AbilityLevel)
{
	float CooldownDuration = 0.f;

	if (AbilityCDO)
	{
		UGameplayEffect* CooldownEffect = AbilityCDO->GetCooldownGameplayEffect();
		if (CooldownEffect)
		{
			FGameplayEffectSpecHandle EffectSpec = ASC.MakeOutgoingSpec(CooldownEffect->GetClass(), AbilityLevel, ASC.MakeEffectContext());
			CooldownEffect->DurationMagnitude.AttemptCalculateMagnitude(*EffectSpec.Data.Get(), CooldownDuration);
		}
	}

	return FMath::Abs(CooldownDuration);
}

float UCAbilitySystemStatics::GetCooldownRemainingFor(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC)
{
	if (!AbilityCDO)
		return 0.f;

	UGameplayEffect* CooldownEffect = AbilityCDO->GetCooldownGameplayEffect();
	if (!CooldownEffect)
		return 0.f;

	FGameplayEffectQuery CooldownEffectQuery;
	CooldownEffectQuery.EffectDefinition = CooldownEffect->GetClass();

	float CooldownRemaining = 0.f;

	FJsonSerializableArrayFloat CooldownTimeRemainings = ASC.GetActiveEffectsTimeRemaining(CooldownEffectQuery);

	for (float Remaining : CooldownTimeRemainings)
	{
		if (Remaining > CooldownRemaining)
		{
			CooldownRemaining = Remaining;
		}
	}

	return CooldownRemaining;
}

void UCAbilitySystemStatics::SendLocalGameplayCue(AActor* CueTargetActor, const FHitResult& HitResult, const FGameplayTag& GameplayCueTag)
{
	FGameplayCueParameters CueParams;
	CueParams.Location = HitResult.ImpactPoint;
	CueParams.Normal = HitResult.ImpactNormal;

	UAbilitySystemGlobals::Get().GetGameplayCueManager()->HandleGameplayCue(CueTargetActor, GameplayCueTag, EGameplayCueEvent::Executed, CueParams);
}

void UCAbilitySystemStatics::GetKillFeedIdentity(const AActor* Actor, FString& OutName, TSoftObjectPtr<UTexture2D>& OutIcon, uint8& OutTeamId)
{
	OutName = TEXT("알 수 없음");
	OutIcon.Reset();
	OutTeamId = 0;

	if ( !Actor )
		return;

	if ( const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(Actor) )
	{
		OutTeamId = TeamAgent->GetGenericTeamId().GetId();
	}

	if ( IsHero(Actor) )
	{
		if ( const APawn* Pawn = Cast<APawn>(Actor) )
		{
			if ( const ACPlayerState* CPlayerState = Pawn->GetPlayerState<ACPlayerState>() )
			{
				const FPlayerSelection& Selection = CPlayerState->GetPlayerSelection();
				OutName = Selection.GetPlayerNickName();

				if ( const UPA_CharacterDefinition* Definition = Selection.GetCharacterDefinition() )
				{
					if ( UTexture2D* LoadedIcon = Definition->LoadIcon() )
					{
						OutIcon = LoadedIcon;
					}
				}

				return;
			}
		}
	}

	if ( const ATurret* Turret = Cast<ATurret>(Actor) )
	{
		OutName = Turret->GetKillFeedDisplayName();
		OutIcon = Turret->GetKillFeedIcon();
		
		return;
	}

	if ( const ACCharacter* Char = Cast<ACCharacter>(Actor) )
	{
		OutName = Char->GetKillFeedDisplayName();
		OutIcon = Char->GetKillFeedIcon();
	}
}

ACPlayerState* UCAbilitySystemStatics::GetHeroPlayerState(const AActor* Actor)
{
	if ( const APawn* Pawn = Cast<APawn>(Actor) )
	{
		return Pawn->GetPlayerState<ACPlayerState>();
	}

	return nullptr;
}

FGameplayTag UCAbilitySystemStatics::GetAbilityBaseDamageTag()
{
	return FGameplayTag::RequestGameplayTag("Effect.Magnitude.AbilityBaseDamage");
}

FGameplayTag UCAbilitySystemStatics::GetAbilityScalingRatioTag()
{
	return FGameplayTag::RequestGameplayTag("Effect.Magnitude.AbilityScalingRatio");
}

const FBuffIconData* UCAbilitySystemStatics::FindBuffIconDataForTags(const UDataTable* Table, const FGameplayTagContainer& GrantedTags)
{
	if ( !Table || GrantedTags.IsEmpty() )
		return nullptr;

	for ( const TPair<FName, uint8*>& RowPair : Table->GetRowMap() )
	{
		const FBuffIconData* Row = Table->FindRow<FBuffIconData>(RowPair.Key, TEXT(""));
		if ( Row && Row->StatusTag.IsValid() && GrantedTags.HasTag(Row->StatusTag) )
		{
			return Row;
		}
	}

	return nullptr;
}

FGameplayTag UCAbilitySystemStatics::GetDamagePopupGameplayCueTag()
{
	return FGameplayTag::RequestGameplayTag("GameplayCue.Damage.Popup");
}

FGameplayTag UCAbilitySystemStatics::GetPhysicalDamageTypeTag()
{
	return FGameplayTag::RequestGameplayTag("Damage.Type.Physical");
}

FGameplayTag UCAbilitySystemStatics::GetMagicalDamageTypeTag()
{
	return FGameplayTag::RequestGameplayTag("Damage.Type.Magical");

}

FGameplayTag UCAbilitySystemStatics::GetBlackholeHoldVFXGameplayCueTag()
{
	return FGameplayTag::RequestGameplayTag("GameplayCue.Blackhole.Hold");
}

FGameplayTag UCAbilitySystemStatics::GetDamagedEventTag()
{
	return FGameplayTag::RequestGameplayTag("Event.Damaged");
}

FGameplayTag UCAbilitySystemStatics::GetInBaseStatusTag()
{
	return FGameplayTag::RequestGameplayTag("Status.InBase");
}

FGameplayTag UCAbilitySystemStatics::GetRecallStatusTag()
{
	return FGameplayTag::RequestGameplayTag("GameplayCue.Status.Recall");
}

FGameplayTag UCAbilitySystemStatics::GetHealPopupGameplayCueTag()
{
	return FGameplayTag::RequestGameplayTag("GameplayCue.Heal.Popup");
}

FGameplayTag UCAbilitySystemStatics::GetRegenHealTypeTag()
{
	return FGameplayTag::RequestGameplayTag("Heal.Type.Regen");
}

FGameplayTag UCAbilitySystemStatics::GetLifeStealHealTypeTag()
{
	return FGameplayTag::RequestGameplayTag("Heal.Type.LifeSteal");
}

FGameplayTag UCAbilitySystemStatics::GetTracerGameplayCueTag()
{
	return FGameplayTag::RequestGameplayTag("GameplayCue.Weapon.Tracer");
}

FGameplayTag UCAbilitySystemStatics::GetReloadingStatusTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Type.Reloading");
}

FGameplayTag UCAbilitySystemStatics::GetBasicAttackAimStatusTag()
{
	return FGameplayTag::RequestGameplayTag("Status.Aim.BasicAttack");
}

FGameplayTag UCAbilitySystemStatics::GetSkillAimStatusTag()
{
	return FGameplayTag::RequestGameplayTag("Status.Aim.Skill");
}

bool UCAbilitySystemStatics::WasRecentlyRenderedOnScreen(const UPrimitiveComponent* Component, float Tolerance)
{
	const UWorld* World = Component ? Component->GetWorld() : nullptr;
	if ( !World )
		return false;

	return World->GetTimeSeconds() - Component->GetLastRenderTimeOnScreen() <= Tolerance;
}

FGameplayTag UCAbilitySystemStatics::GetMatchNotStartedStatusTag()
{
	return TAG_Status_MatchNotStarted;
}

FGameplayTag UCAbilitySystemStatics::GetMurdockHitEffectsGameplayCueTag()
{
	return FGameplayTag::RequestGameplayTag("GameplayCue.Hit.Murdock.Shotgun");
}

FGameplayTag UCAbilitySystemStatics::GetMurdockBlastHitEffectsGameplayCueTag()
{
	return FGameplayTag::RequestGameplayTag("GameplayCue.Hit.Murdock.Blast");
}

FGameplayTag UCAbilitySystemStatics::GetMurdockLaserSightGameplayCueTag()
{
	return FGameplayTag::RequestGameplayTag("GameplayCue.Murdock.LaserSight");
}

FGameplayTag UCAbilitySystemStatics::GetMurdockBlastBeamGameplayCueTag()
{
	return FGameplayTag::RequestGameplayTag("GameplayCue.Murdock.BlastBeam");
}

FGameplayTag UCAbilitySystemStatics::GetShieldBuffTag()
{
	return FGameplayTag::RequestGameplayTag("Status.Buff.Shield");
}

FGameplayTag UCAbilitySystemStatics::GetShieldMagnitudeTag()
{
	return FGameplayTag::RequestGameplayTag("Effect.Magnitude.Shield");
}

void UCAbilitySystemStatics::ApplyTimedShield(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, TSubclassOf<class UGameplayEffect> ShieldGrantEffect, TSubclassOf<class UGameplayEffect> ShieldBuffIconEffect, int32 EffectLevel, float ShieldDuration, float ShieldAmount)
{
	if ( !SourceASC || !TargetASC || !ShieldGrantEffect || !ShieldBuffIconEffect )
		return;

	if ( ACCharacter* TargetCharacter = Cast<ACCharacter>(TargetASC->GetAvatarActor()) )
	{
		TargetCharacter->RecordShieldProvider(SourceASC->GetAvatarActor());
	}
	
	bool bFound = false;
	float ShieldBefore = TargetASC->GetGameplayAttributeValue(UCAttributeSet::GetShieldAttribute(), bFound);

	FGameplayEffectSpecHandle GrantSpec = SourceASC->MakeOutgoingSpec(ShieldGrantEffect, EffectLevel, SourceASC->MakeEffectContext());
	if ( GrantSpec.IsValid() )
	{
		if ( ShieldAmount >= 0.f )
		{
			GrantSpec.Data->SetSetByCallerMagnitude(GetShieldMagnitudeTag(), ShieldAmount);
		}

		TargetASC->ApplyGameplayEffectSpecToSelf(*GrantSpec.Data.Get());
	}

	float ShieldAfter = TargetASC->GetGameplayAttributeValue(UCAttributeSet::GetShieldAttribute(), bFound);
	float GrantedAmount = FMath::Max(ShieldAfter - ShieldBefore, 0.f); 

	FGameplayEffectSpecHandle IconSpec = SourceASC->MakeOutgoingSpec(ShieldBuffIconEffect, 1, SourceASC->MakeEffectContext());
	if ( IconSpec.IsValid() )
	{
		IconSpec.Data->SetDuration(ShieldDuration, true);
		FActiveGameplayEffectHandle IconHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*IconSpec.Data.Get());

		ACCharacter* TargetCharacter = Cast<ACCharacter>(TargetASC->GetAvatarActor());
		if ( TargetCharacter )
		{
			TargetCharacter->AddShieldBuffHandle(IconHandle);
		}

		if ( FOnActiveGameplayEffectRemoved_Info* RemovedDelegate = TargetASC->OnGameplayEffectRemoved_InfoDelegate(IconHandle) )
		{
			TWeakObjectPtr<UAbilitySystemComponent> WeakTargetASC = TargetASC;
			TWeakObjectPtr<ACCharacter> WeakTargetCharacter = TargetCharacter;
			FActiveGameplayEffectHandle HandleCopy = IconHandle;
			float ThisGrantedAmount = GrantedAmount;

			RemovedDelegate->AddLambda([ WeakTargetASC, WeakTargetCharacter, HandleCopy, ThisGrantedAmount ] (const FGameplayEffectRemovalInfo& RemovalInfo)
				{
					if ( !WeakTargetASC.IsValid() )
						return;

					if ( WeakTargetCharacter.IsValid() )
					{
						WeakTargetCharacter->RemoveShieldBuffHandle(HandleCopy);
					}

					bool bStillFound = false;
					float CurrentShield = WeakTargetASC->GetGameplayAttributeValue(UCAttributeSet::GetShieldAttribute(), bStillFound);
					if ( bStillFound && CurrentShield > 0.f )
					{
						float NewShield = FMath::Max(CurrentShield - ThisGrantedAmount, 0.f);
						WeakTargetASC->SetNumericAttributeBase(UCAttributeSet::GetShieldAttribute(), NewShield);
					}
				});
		}
	}

	UCItemStatics::HandleAllySupported(SourceASC, TargetASC);
}

FGameplayTag UCAbilitySystemStatics::GetLandingMarkerGameplayCueTag()
{
	return FGameplayTag::RequestGameplayTag("GameplayCue.Muriel.LandingMarker");
}

FGameplayTag UCAbilitySystemStatics::GetDamageStrengthRatioTag()
{
	return FGameplayTag::RequestGameplayTag("Damage.Scale.StrengthRatio");
}

FGameplayTag UCAbilitySystemStatics::GetDamageIntelligenceRatioTag()
{
	return FGameplayTag::RequestGameplayTag("Damage.Scale.IntelligenceRatio");
}

FGameplayTag UCAbilitySystemStatics::GetInvincibleStatusTag()
{
	return FGameplayTag::RequestGameplayTag("Status.Invincible");
}

FGameplayTag UCAbilitySystemStatics::GetCCImmuneStatusTag()
{
	return FGameplayTag::RequestGameplayTag("Status.CCImmune");
}

FGameplayTag UCAbilitySystemStatics::GetBasicAttackTypeTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Type.BasicAttack");
}

FGameplayTag UCAbilitySystemStatics::GetGiantSlayerEquippedTag()
{
	return FGameplayTag::RequestGameplayTag("Item.GiantSlayer.Equipped");
}

FGameplayTag UCAbilitySystemStatics::GetMaxHealthPercentDataTag()
{
	return FGameplayTag::RequestGameplayTag("Data.MaxHealthPercent");
}

FGameplayTag UCAbilitySystemStatics::GetStepOfHermesEquippedTag()
{
	return FGameplayTag::RequestGameplayTag("Item.Boots.Hermes.Equipped");
}

FGameplayTag UCAbilitySystemStatics::GetArchmageSealEquippedTag()
{
	return FGameplayTag::RequestGameplayTag("Item.ArchmageSeal.Equipped");
}

FGameplayTag UCAbilitySystemStatics::GetIronCitadelEquippedTag()
{
	return FGameplayTag::RequestGameplayTag("Item.IronCitadel.Equipped");
}

FGameplayTag UCAbilitySystemStatics::GetOnlyBasicAttackDamageTag()
{
	return FGameplayTag::RequestGameplayTag("Status.OnlyBasicAttackDamage");
}

FGameplayTag UCAbilitySystemStatics::GetCrowdControlledStatusTag()
{
	return FGameplayTag::RequestGameplayTag("Status.CrowdControlled");
}

bool UCAbilitySystemStatics::GetBasicAttackDamageAgainst(const UAbilitySystemComponent* SourceASC, const UAbilitySystemComponent* TargetASC, float& OutDamage)
{
	OutDamage = 0.f;

	if ( !SourceASC || !TargetASC )
		return false;

	float RawDamage = 0.f;
	bool bIsMagical = false;
	bool bFoundPreview = false;

	const int32 CandidateInputIDs[] = { static_cast< int32 >(ECAbilityInputID::BasicAttack), static_cast< int32 >(ECAbilityInputID::Aim) };

	for ( const int32 InputID : CandidateInputIDs )
	{
		for ( const FGameplayAbilitySpec& Spec : SourceASC->GetActivatableAbilities() )
		{
			if ( Spec.InputID != InputID )
				continue;

			const UCGameplayAbility* Ability = Cast<UCGameplayAbility>(Spec.Ability);
			if ( Ability && Ability->GetBasicAttackDamagePreview(SourceASC, RawDamage, bIsMagical) )
			{
				bFoundPreview = true;
				break;
			}
		}

		if ( bFoundPreview )
			break;
	}

	if ( !bFoundPreview )
		return false;

	bool bFound = false;
	float Defense = TargetASC->GetGameplayAttributeValue(
		bIsMagical ? UCAttributeSet::GetMagicResistanceAttribute() : UCAttributeSet::GetArmorAttribute(), bFound);

	FGameplayTagContainer SourceOwnedTags;
	SourceASC->GetOwnedGameplayTags(SourceOwnedTags);
	Defense *= UCItemStatics::GetDefensePenetrationMultiplier(&SourceOwnedTags, bIsMagical);

	OutDamage = UMMC_AbilityDamage::ApplyDefense(RawDamage, Defense);

	if ( SourceASC->HasMatchingGameplayTag(GetGiantSlayerEquippedTag()) && CanApplyGiantSlayerTo(TargetASC->GetAvatarActor()) )
	{
		const float Armor = TargetASC->GetGameplayAttributeValue(UCAttributeSet::GetArmorAttribute(), bFound);
		const float MaxHealth = TargetASC->GetGameplayAttributeValue(UCAttributeSet::GetMaxHealthAttribute(), bFound);

		OutDamage += UMMC_AbilityDamage::ApplyDefense(MaxHealth * GiantSlayerMaxHealthRatio, Armor);
	}

	return true;
}

FGameplayTag UCAbilitySystemStatics::GetItemBonusDamageTag()
{
	return FGameplayTag::RequestGameplayTag("Damage.Source.Item");
}

bool UCAbilitySystemStatics::CanApplyGiantSlayerTo(const AActor* Victim)
{
	return Victim && !Victim->IsA<ATurret>();
}
