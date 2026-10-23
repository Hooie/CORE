// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayEffect.h"
#include "CGameplayAbilityTypes.generated.h"

UENUM(BlueprintType)
enum class ECAbilityInputID : uint8
{
	None							UMETA(DisplayName = "None"),
	BasicAttack						UMETA(DisplayName = "Basic Attack"),
	Aim								UMETA(DisplayName = "Aim"),
	AbilityOne						UMETA(DisplayName = "Ability One"),
	AbilityTwo						UMETA(DisplayName = "Ability Two"),
	AbilityThree					UMETA(DisplayName = "Ability Three"),
	Ultimate						UMETA(DisplayName = "Ultimate"),
	Recall							UMETA(DisplayName = "Recall"),
	Reload							UMETA(DisplayName = "Reload"),

	Confirm							UMETA(DisplayName = "Confirm"),
	Cancel							UMETA(DisplayName = "Cancel")
};

USTRUCT(BlueprintType)
struct FGenericDamageEffectDef
{
	GENERATED_BODY()

public:
	FGenericDamageEffectDef();	
	
public:
	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditAnywhere)
	FVector PushVelocity;

	UPROPERTY(EditAnywhere)
	TArray<float> BaseDamagePerLevel;

	UPROPERTY(EditAnywhere)
	float ScalingRatio;
};

USTRUCT(BlueprintType)
struct FHeroBaseStats : public FTableRowBase
{
	GENERATED_BODY()

public:
	FHeroBaseStats();

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> Class;

	UPROPERTY(EditAnywhere)
	float Strength;

	UPROPERTY(EditAnywhere)
	float StrengthGrowthRate;

	UPROPERTY(EditAnywhere)
	float Intelligence;

	UPROPERTY(EditAnywhere)
	float IntelligenceGrowthRate;

	UPROPERTY(EditAnywhere)
	float BaseMaxHealth;

	UPROPERTY(EditAnywhere)
	float MaxHealthGrowthRate;

	UPROPERTY(EditAnywhere)
	float BaseMaxMana;

	UPROPERTY(EditAnywhere)
	float MaxManaGrowthRate;

	UPROPERTY(EditAnywhere)
	float BaseHealthRegen;

	UPROPERTY(EditAnywhere)
	float HealthRegenGrowthRate;

	UPROPERTY(EditAnywhere)
	float BaseManaRegen;

	UPROPERTY(EditAnywhere)
	float ManaRegenGrowthRate;

	UPROPERTY(EditAnywhere)
	float BaseArmor;

	UPROPERTY(EditAnywhere)
	float ArmorGrowthRate;

	UPROPERTY(EditAnywhere)
	float BaseMagicResistance;

	UPROPERTY(EditAnywhere)
	float MagicResistanceGrowthRate;

	UPROPERTY(EditAnywhere)
	float BaseMoveSpeed;

	UPROPERTY(EditAnywhere)
	float BaseMoveAcceleration;	
};

USTRUCT(BlueprintType)
struct FBuffIconData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FGameplayTag StatusTag;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere)
	bool bIsDebuff = false;
};

USTRUCT(BlueprintType)
struct FMinionStatScaling
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float StartTimeSeconds = 0.f;

	UPROPERTY(EditAnywhere)
	float IntervalSeconds = 90.f;

	UPROPERTY(EditAnywhere)
	float IncrementPerInterval = 0.f;

	UPROPERTY(EditAnywhere)
	float CapTimeSeconds = 1800.f;

	float GetBonus(float ElapsedSeconds) const
	{
		if ( IntervalSeconds <= 0.f )
			return 0.f;

		float EffectiveElapsed = FMath::Min(ElapsedSeconds, CapTimeSeconds);
		if ( EffectiveElapsed < StartTimeSeconds )
			return 0.f;

		int32 Steps = FMath::FloorToInt((EffectiveElapsed - StartTimeSeconds) / IntervalSeconds) + 1;
		return Steps * IncrementPerInterval;
	}
};

USTRUCT(BlueprintType)
struct FMinionScalingConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FMinionStatScaling HealthScaling;

	UPROPERTY(EditAnywhere)
	FMinionStatScaling StrengthScaling;

	UPROPERTY(EditAnywhere)
	FMinionStatScaling ArmorScaling;

	UPROPERTY(EditAnywhere)
	FMinionStatScaling MoveSpeedScaling;
};