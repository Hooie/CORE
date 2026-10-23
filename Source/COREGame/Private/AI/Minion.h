// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/CCharacter.h"
#include "Minion.generated.h"

/**
 * 
 */
UCLASS()
class AMinion : public ACCharacter
{
	GENERATED_BODY()

public:
	AMinion();

	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;

	virtual void BeginPlay() override;

	bool IsActive() const;
	void Activate();

private:
	void PickSkinBasedOnTeamID();

	virtual void OnRep_TeamID() override;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TMap<FGenericTeamId, USkeletalMesh*> SkinMap;

	UPROPERTY(VisibleDefaultsOnly, Category = "Gameplay Ability")
	class UCHeroAttributeSet* HeroAttributeSet;

	/******************************/
	/************Scaling***********/
	/******************************/
	UPROPERTY(EditDefaultsOnly, Category = "Scaling")
	FMinionScalingConfig ScalingConfig;

	UPROPERTY(EditDefaultsOnly, Category = "Scaling")
	float ScalingUpdateInterval = 5.f;

	FTimerHandle ScalingTimerHandle;

	void UpdateScaling();

	/******************************/
	/*************Lane*************/
	/******************************/
public:
	void SetLane(class ALaneWayPointPath* InPath, bool bInReverse);

	bool GetLaneMoveGoal(FVector& OutGoal);

private:
	void IgnoreOtherMinionsWhenMoving();

	float ToPathDistance(float Progress, float Total) const { return bReverseLane ? Total - Progress : Progress; }

	UPROPERTY()
	class ALaneWayPointPath* LanePath = nullptr;

	bool bReverseLane = false;

	float LaneProgress = 0.f;

	// 현재 진행도보다 이만큼 앞의 지점을 목표로 걸음 (클수록 코너링 부드러움)
	UPROPERTY(EditDefaultsOnly, Category = "AI|Lane")
	float LaneLookAhead = 500.f;

	// 진행도를 다시 계산할 때 직전 진행도 기준으로 뒤 몇 cm까지 찾을지
	UPROPERTY(EditDefaultsOnly, Category = "AI|Lane")
	float ProgressSearchBack = 400.f;

	// 진행도를 다시 계산할 때 직전 진행도 기준으로 앞 몇 cm까지 찾을지
	UPROPERTY(EditDefaultsOnly, Category = "AI|Lane")
	float ProgressSearchForward = 1200.f;

	/******************************/
	/************Combat************/
	/******************************/
public:
	float GetAttackRange() const { return AttackRange; }
	float GetAttackInterval() const { return AttackInterval; }
	ECAbilityInputID GetAttackInputID() const { return AttackInputID; }

private:
	// 내 캡슐 가장자리 ~ 대상 캡슐 가장자리 거리 기준
	UPROPERTY(EditDefaultsOnly, Category = "AI|Combat")
	float AttackRange = 150.f;

	// 공격 입력을 보내는 최소 간격 (초)
	UPROPERTY(EditDefaultsOnly, Category = "AI|Combat")
	float AttackInterval = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "AI|Combat")
	ECAbilityInputID AttackInputID = ECAbilityInputID::BasicAttack;
};