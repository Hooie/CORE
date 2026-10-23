// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "GenericTeamAgentInterface.h"
#include "TargetActor_Around.generated.h"

/**
 * 
 */
UCLASS()
class ATargetActor_Around : public AGameplayAbilityTargetActor, public IGenericTeamAgentInterface
{
	GENERATED_BODY()
	
public:
	ATargetActor_Around();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void SetGenericTeamId(const FGenericTeamId& InTeamID) override;
	FORCEINLINE virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }

	void ConfigureDetection(float InDetectionRadius, const FGenericTeamId& InTeamId, const FGameplayTag& InLocalGameplayCueTag);

private:
	UPROPERTY(Replicated)
	FGenericTeamId TeamId;

	UPROPERTY(VisibleDefaultsOnly, Category = "Comp")
	class USceneComponent* RootComp;

	UPROPERTY(VisibleDefaultsOnly, Category = "Comp")
	class USphereComponent* DetectionSphere;

	UPROPERTY(ReplicatedUsing = OnRep_TargetDetectionRadiusReplicated)
	float TargetDetectionRadius;

	UFUNCTION()
	void OnRep_TargetDetectionRadiusReplicated();

	UPROPERTY(Replicated)
	FGameplayTag LocalGameplayCueTag;

	UFUNCTION()
	void ActorInDectionRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
