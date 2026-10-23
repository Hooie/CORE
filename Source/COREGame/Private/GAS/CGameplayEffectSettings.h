// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "CGameplayEffectSettings.generated.h"

/**
 * 
 */

UCLASS(Config = Game, DefaultConfig, meta=(DisplayName = "Gameplay Effect Settings"))
class UCGameplayEffectSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	/***********************/
	/*******Life Steal******/
	/***********************/
	UPROPERTY(Config, EditAnywhere, Category = "LifeSteal", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MultiHitLifeStealRatio = 0.f;


	/***********************/
	/*********Item**********/
	/***********************/

	//이 태그가 붙은 아이템은 각 카테고리 당 한개씩만 구매 가능
	UPROPERTY(Config, EditAnywhere, Category = "Shop", meta = (Categories = "ItemCategory"))
	FGameplayTagContainer ExclusiveItemCategories;							
	
	//거인학살자 추가뎀 데미지 이펙트
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|GiantSlayer")
	TSoftClassPtr<class UGameplayEffect> GiantSlayerBonusDamageEffect;		
	

	/***********************/
	/*****Item Passives*****/
	/***********************/

	//아이템의 마법데미지 이펙트
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Common")
	TSoftClassPtr<class UGameplayEffect> ItemMagicDamageEffect;				

	//아이템 힐 이펙트
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Common")	
	TSoftClassPtr<class UGameplayEffect> ItemHealEffect;					

	//생명력흡수 이펙트
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Common")
	TSoftClassPtr<class UGameplayEffect> LifeStealHealEffect;				

	/*  적중 시 아이템 이펙트 */

	//용사의 도끼 이동속도 증가 이펙트
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|OnHit")
	TSoftClassPtr<class UGameplayEffect> HeroAxeHasteEffect;			

	//광전사의 신발 이동속도 증가 이펙트
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|OnHit")
	TSoftClassPtr<class UGameplayEffect> BerserkerHasteEffect;			
	
	//흑색 도끼 방깎 이펙트
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|OnHit")
	TSoftClassPtr<class UGameplayEffect> BlackCleaverShredEffect;		

	//서리 수정구 둔화 이펙트
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|OnHit")
	TSoftClassPtr<class UGameplayEffect> FrostOrbSlowEffect;			

	//대자연의 지팡이 버프 이펙트
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Support")
	TSoftClassPtr<class UGameplayEffect> GreatNatureBuffEffect;			

	//폭주 마력구 쿨다운 이펙트
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|OnHit")
	TSoftClassPtr<UGameplayEffect> ArcaneSurgeCooldownEffect;

	//자연의 수호갑 마저 중첩 이펙트 
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|OnHit")
	TSoftClassPtr<UGameplayEffect> NatureGuardStackEffect;

	/*  각종 아이템 수치  */

	//대마법사의 인장 스킬 피해 증폭 계수
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Values")
	float ArchmageSealSkillAmp = 0.2f;									

	//초심자의검 미니언 처치 추가 골드
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Values")
	float StarterBladeBonusGold = 1.f;									

	//수호자의성배 추가 골드
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Values")
	float GuardianGrailBonusGold = 5.f;									

	//초심자의 법전 마나 환원 계수
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Values")
	float StarterTomeManaRefundRatio = 0.05f;							

	//갑옷분쇄자 방관 계수
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Values")
	float ArmorPenetrationMinor = 0.15f;								

	//파멸의 창 방관 계수
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Values")
	float ArmorPenetrationMajor = 0.35f;								

	//균열의 보석 마관 계수
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Values")
	float MagicPenetrationMinor = 0.13f;								

	//공허의 지팡이 마관 계수
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Values")
	float MagicPenetrationMajor = 0.35f;								

	//처형자의 대낫 추가 데미지 계수
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Values")
	float ExecutionerDamageAmp = 0.15f;									

	//판금장화 기본공격 데미지 감소 계수
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Values")
	float PlatedBootsBasicAttackReduction = 0.1f;						

	// 얼음 군주의 성벽 기본 공격 데미지 감소 계수
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Values")
	float FrostWardenBasicAttackReduction = 0.1f;

	//헤르메스의 발걸음 CC 유지 지속시간 계수
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Values")
	float HermesCCDurationMultiplier = 0.75f;							

	//흡혈검 영웅 처치 최대체력 비례 힐 계수
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Values")
	float BloodthirsterKillHealRatio = 0.1f;							

	//가시갑옷 기본 데미지
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Values")
	float ThornmailBaseDamage = 20.f;									

	//가시갑옷 방어력 비례 피해량 계수
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Values")
	float ThornmailArmorRatio = 0.25f;									

	//폭주 마력구 기본 데미지
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Values")
	float ArcaneSurgeBaseDamage = 80.f;

	//폭주 마력구 주문력 계수
	UPROPERTY(Config, EditAnywhere, Category = "Item Effects|Values")
	float ArcaneSurgeIntelligenceRatio = 0.15f;

	/***********************/
	/**********UI***********/
	/***********************/
	UPROPERTY(Config, EditAnywhere, Category = "UI")
	TMap<FGameplayAttribute, FText> AttributeDisplayNames;

	UPROPERTY(Config, EditAnywhere, Category = "UI")
	TMap<FGameplayAttribute, FText> PercentAttributeDisplayNames;

	UPROPERTY(Config, EditAnywhere, Category = "UI")
	TArray<FGameplayAttribute> RatioAttributes;
};
