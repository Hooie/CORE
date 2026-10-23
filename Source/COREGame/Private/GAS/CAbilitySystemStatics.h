// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GAS/CGameplayAbilityTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CAbilitySystemStatics.generated.h"

class UGameplayAbility;
struct FGameplayAbilitySpec;
class UAbilitySystemComponent;

/**
 * 
 */
UCLASS()
class UCAbilitySystemStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/***************************************/
	/****************Ability****************/
	/***************************************/
	static FGameplayTag GetBasicAttackAbilityTag();
	static FGameplayTag GetBasicAttackInputPressedTag();
	static FGameplayTag GetBasicAttackInputReleasedTag();
	
	static FGameplayTag GetGenericDamageTag();

	static FGameplayTag GetRecallSwordEventTag();
	static FGameplayTag GetActivateEventTag();

	static FGameplayTag GetPassiveAbilityTypeTag();

	static bool IsAbilityAtMaxLevel(const FGameplayAbilitySpec& Spec);
	static const int32 UltimateMaxLevel;         // = 3
	static const int32 DefaultAbilityMaxLevel;   // = 5
	static const int32 UltRequiredLevel[ 3 ];

	static float GetStaticCooldownDurationForAbility(const UGameplayAbility* Ability);
	static float GetStaticCostForAbility(const UGameplayAbility* Ability);

	static bool CheckCost(const FGameplayAbilitySpec& AbilitySpec, const UAbilitySystemComponent& ASC);
	static bool CheckAbilityCostStatic(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC);
	static float GetManaCostFor(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC, int AbilityLevel);
	static float GetCooldownDurationFor(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC, int AbilityLevel);
	static float GetCooldownRemainingFor(const UGameplayAbility* AbilityCDO, const UAbilitySystemComponent& ASC);

	/***************************************/
	/*****************State*****************/
	/***************************************/
	static FGameplayTag GetDeadStatusTag();
	static FGameplayTag GetStunStatusTag();
	static FGameplayTag GetDisarmedTag();
	static FGameplayTag GetBindStatusTag();
	static FGameplayTag GetDisableJumpTag();

	static FGameplayTag GetAimStatusTag();
	static FGameplayTag GetFocusStatusTag();
	static FGameplayTag GetCrosshairTag();
	static FGameplayTag GetHealthFullStatTag();
	static FGameplayTag GetHealthEmptyStatTag();
	static FGameplayTag GetManaFullStatTag();
	static FGameplayTag GetManaEmptyStatTag();

	static FGameplayTag GetExperienceAttributeTag();
	static FGameplayTag GetGoldAttributeTag();

	static FGameplayTag GetLifeStealStatusTag();

	static FGameplayTag GetNoWeaponTag();

	static FGameplayTag GetLevelupTag();

	static FGameplayTag GetHeroRoleTag();

	static bool ActorHasTag(const AActor* ActorToCheck, const FGameplayTag& Tag);
	static bool IsActorDead(const AActor* ActorToCheck);

	static bool IsHero(const AActor* ActorToCheck);

	static class ACPlayerState* GetHeroPlayerState(const AActor* Actor);

	static FGameplayTag GetInBaseStatusTag();
	static FGameplayTag GetRecallStatusTag();

	static FGameplayTag GetReloadingStatusTag();

	static FGameplayTag GetBasicAttackAimStatusTag();
	static FGameplayTag GetSkillAimStatusTag();
	
	static bool WasRecentlyRenderedOnScreen(const class UPrimitiveComponent* Component, float Tolerance);

	static FGameplayTag GetMatchNotStartedStatusTag();

	/**************************************/
	/****************Combat****************/
	/**************************************/
	static FGameplayTag GetTargetUpdatedTag();

	static FGameplayTag GetLifeStealProcessTag();
	static FGameplayTag GetHealMagnitudeTag();

	static FGameplayTag GetAbilityBaseDamageTag();
	static FGameplayTag GetAbilityScalingRatioTag();

	static FGameplayTag GetPhysicalDamageTypeTag();
	static FGameplayTag GetMagicalDamageTypeTag();

	static void GetKillFeedIdentity(const AActor* Actor, FString& OutName, TSoftObjectPtr<UTexture2D>& OutIcon, uint8& OutTeamId);

	static const FBuffIconData* FindBuffIconDataForTags(const UDataTable* Table, const FGameplayTagContainer& GrantedTags);

	static FGameplayTag GetDamagedEventTag();

	static FGameplayTag GetRegenHealTypeTag();
	static FGameplayTag GetLifeStealHealTypeTag();

	static FGameplayTag GetShieldBuffTag();
	static FGameplayTag GetShieldMagnitudeTag();
	static void ApplyTimedShield(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, TSubclassOf<class UGameplayEffect> ShieldGrantEffect, TSubclassOf<class UGameplayEffect> ShieldBuffIconEffect, int32 EffectLevel, float ShieldDuration, float ShieldAmount = -1.f);

	static FGameplayTag GetDamageStrengthRatioTag();
	static FGameplayTag GetDamageIntelligenceRatioTag();

	static FGameplayTag GetInvincibleStatusTag();
	static FGameplayTag GetCCImmuneStatusTag();

	static FGameplayTag GetBasicAttackTypeTag();
	
	static FGameplayTag GetMaxHealthPercentDataTag();

	static FGameplayTag GetOnlyBasicAttackDamageTag();

	static FGameplayTag GetCrowdControlledStatusTag();

	static bool GetBasicAttackDamageAgainst(const class UAbilitySystemComponent* SourceASC, const class UAbilitySystemComponent* TargetASC, float& OutDamage);

	static FGameplayTag GetItemBonusDamageTag();

	static bool CanApplyGiantSlayerTo(const AActor* Victim);

	static constexpr float GiantSlayerMaxHealthRatio = 0.03f;


	/***************************************/
	/**************GameplayCue**************/
	/***************************************/
	static FGameplayTag GetCameraShakeGameplayCueTag();
	static void SendLocalGameplayCue(AActor* CueTargetActor, const FHitResult& HitResult, const FGameplayTag& GameplayCueTag);
	static FGameplayTag GetDamagePopupGameplayCueTag();
	static FGameplayTag GetBlackholeHoldVFXGameplayCueTag();
	static FGameplayTag GetHealPopupGameplayCueTag();
	
	static FGameplayTag GetTracerGameplayCueTag();

	static FGameplayTag GetMurdockHitEffectsGameplayCueTag();
	static FGameplayTag GetMurdockBlastHitEffectsGameplayCueTag();
	static FGameplayTag GetMurdockLaserSightGameplayCueTag();
	static FGameplayTag GetMurdockBlastBeamGameplayCueTag();

	static FGameplayTag GetLandingMarkerGameplayCueTag();

	/**************************************/
	/*****************Item*****************/
	/**************************************/
	static FGameplayTag GetGiantSlayerEquippedTag();
	static FGameplayTag GetStepOfHermesEquippedTag();
	static FGameplayTag GetArchmageSealEquippedTag();
	static FGameplayTag GetIronCitadelEquippedTag();
	
};
