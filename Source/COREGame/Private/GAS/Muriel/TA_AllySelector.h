// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "TA_AllySelector.generated.h"

/**
 * 
 */
UCLASS()
class ATA_AllySelector : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()
	
public:
	ATA_AllySelector();
	
	virtual void Tick(float DeltaTime) override;
	virtual void ConfirmTargetingAndContinue() override;

	void SetSelectionRules(float InMaxTargetRange, bool bInAllowSelfTarget);

private:
	UPROPERTY(VisibleAnywhere, Category = "Visual")
	class UStaticMeshComponent* IndicatorMesh; 

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	float IndicatorRadius = 60.f; 

	// 이 값이 0이면 사거리 무한
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float MaxTargetRange = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	bool bAllowSelfTarget = false;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	bool bRequireBeInSight = true;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TEnumAsByte<ECollisionChannel> SightBlockingType = ECC_WorldStatic;

	// 유효하지 않은 타겟으로 확정 시도 시 true : 어빌리티 취소 / false : 계속 타게팅
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	bool bCancelOnInvalidConfirm = false;

	UPROPERTY()
	AActor* CurrentSelectedAlly;

	AActor* FindNearestAllyToScreenCenter() const;
	
	bool IsInSight(const AActor* Viewer, const AActor* Target) const;

	void HandleInvalidConfirm();
};
