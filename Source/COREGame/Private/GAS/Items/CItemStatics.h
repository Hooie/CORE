// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "NativeGameplayTags.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CItemStatics.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
struct FGameplayEffectSpec;
struct FGameplayEffectModCallbackData;


UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Passive_StarterBlade);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Passive_StarterTome);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Passive_GuardianGrail);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Passive_HeroAxe);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Passive_ArmorPen_Minor);	// 갑옷 분쇄기	: 방어 관통 (작음)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Passive_ArmorPen_Major);	// 파멸의 창		: 방어 관통 (큼)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Passive_MagicPen_Minor);	// 균열의 보석	: 마저 관통 (작음)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Passive_MagicPen_Major);	// 공허의 지팡이	: 마저 관통 (큼)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Passive_Berserker);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Passive_PlatedBoots);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Passive_Bloodthirster);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Passive_Executioner);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Passive_BlackCleaver);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Passive_FrostOrb);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Passive_ThornMail);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Passive_GreatNature);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Passive_ArcaneSurge);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Cooldown_Item_ArcaneSurge);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Passive_NatureGuard);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Item_Passive_FrostWarden);

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_ItemCategory_Starter);

// 체력 구간 태그 (서버에서만 LooseTag로 붙임)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Health_Below30);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Health_Below40);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Health_Below50);


struct FPendingLifeSteal
{
	float Sum = 0.f;
	float Max = 0.f;
};

/**
 * 
 */
UCLASS()
class UCItemStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	// 체력 구간 계산
	static void UpdateHealthThresholdTags(UAbilitySystemComponent* ASC, float Health, float MaxHealth);

	static float GetDefensePenetrationMultiplier(const FGameplayTagContainer* SourceTags, bool bIsMagical);
	static float GetDamageMultiplier(const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, bool bIsBasicAttack);

	static void HandleDamageDealt(const FGameplayEffectModCallbackData& Data, UAbilitySystemComponent* VictimASC, float DamageDealt);
	static void HandleAllySupported(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC);

	static bool IsAbilityCostEffect(const FGameplayEffectSpec& Spec);
	static float GetManaRefund(const UAbilitySystemComponent* ASC, float ManaSpent);

	static float GetBonusKillGold(const AActor* Killer, bool bVictimIsHero);
	static float GetBonusNonKillerGold(const AActor* Receiver);
	static void HandleHeroKill(AActor* Killer);

	static void ApplyItemMagicDamage(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float BaseDamage);
	static void ApplyItemHeal(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, float Amount, bool bIsLifeSteal);
	static void ApplyItemEffect(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, const TSoftClassPtr<UGameplayEffect>& Effect);

	static void BeginLifeStealBatch(UAbilitySystemComponent* SourceASC);
	static void EndLifeStealBatch(UAbilitySystemComponent* SourceASC);

private:
	static TSubclassOf<UGameplayEffect> LoadItemEffect(const TSoftClassPtr<UGameplayEffect>& Effect);
	static void SetLooseTagState(UAbilitySystemComponent* ASC, const FGameplayTag& Tag, bool bShouldHave, bool bReplicate);
	static bool IsStructure(const AActor* Actor);

	static TMap<TWeakObjectPtr<UAbilitySystemComponent>, FPendingLifeSteal> PendingLifeSteal;
};
