// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "GenericTeamAgentInterface.h"
#include "TA_Blackhole.generated.h"

/**
 * 
 */
UCLASS()
class ATA_Blackhole : public AGameplayAbilityTargetActor, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	ATA_Blackhole();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	void ConfigureBlackhole(float InBlackholeRange, float InPullSpeed, float InBlackholeDuration, const FGenericTeamId& InTeamId);

	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	FORCEINLINE virtual FGenericTeamId GetGenericTeamId() const { return TeamId; }
	
	virtual void StartTargeting(class UGameplayAbility* Ability) override;
	virtual void Tick(float DeltaTime) override;
	virtual void ConfirmTargetingAndContinue() override;
	virtual void CancelTargeting() override;

	void ForceStop();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY(VisibleDefaultsOnly, Category = "Detection")
	class UDecalComponent* GroundDecalComponent;

	UPROPERTY(Replicated)
	FGenericTeamId TeamId;

	float PullSpeed;
	float BlackholeDuration;

	FTimerHandle BlackholeDurationTimerHandle;

	bool bHasStopped = false;

	UPROPERTY(ReplicatedUsing = OnRep_BlackholeRange)
	float BlackholeRange;

	UFUNCTION()
	void OnRep_BlackholeRange();

	UPROPERTY(VisibleDefaultsOnly, Category = "Component")
	FName BlackholeVFXOriginVariableName = "Origin";

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	class UNiagaraSystem* BlackholeLinkVFX;

	UPROPERTY(VisibleDefaultsOnly, Category = "Component")
	class USceneComponent* RootComp;

	UPROPERTY(VisibleDefaultsOnly, Category = "Component")
	class USphereComponent* DetectionSphereComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = "Component")
	class UParticleSystemComponent* VFXComponent;

	UFUNCTION()
	void ActorInBlackholeRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void ActorLeftBlackholeRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void TryAddTarget(AActor* OtherTarget);
	void RemoveTarget(AActor* OtherTarget);

	TMap<AActor*, class UNiagaraComponent*> ActorsInRangeMap;

	void StopBlackhole();
};
