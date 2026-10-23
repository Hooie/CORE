// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "GenericTeamAgentInterface.h"
#include "TA_Sword.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnFuryDisabled);

/**
 * 
 */
UCLASS()
class ATA_Sword : public AGameplayAbilityTargetActor, public IGenericTeamAgentInterface
{
	GENERATED_BODY()
	
public:
	FOnFuryDisabled OnFuryDisabled;

	ATA_Sword();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ConfigureSword(float InSwordRange, float InSwordBindInterval, float InSwordDuration, const FGenericTeamId& InTeamId);

	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	FORCEINLINE virtual FGenericTeamId GetGenericTeamId() const { return TeamId; }

	virtual void StartTargeting(class UGameplayAbility* Ability) override;
	virtual void ConfirmTargetingAndContinue() override;
	virtual void CancelTargeting() override;
	
	void BeginBindTimer();
	void SetSwordLocation(const FVector& NewLocation);

	UPROPERTY(ReplicatedUsing = OnRep_SwordLocation)
	FVector ReplicatedLocation = FVector::ZeroVector;

	void EarlyTerminate();
	void ForceStop();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	UPROPERTY(Replicated)
	FGenericTeamId TeamId;

	float SwordBindInterval;
	FTimerHandle SwordBindIntervalTimerHandle;

	float SwordDurtaion;
	FTimerHandle SwordDurationTimerHandle;

	FTimerHandle FuryDisableTimerHandle;
	void DisableFury();
	
	UPROPERTY(ReplicatedUsing = OnRep_SwordRange)
	float SwordRange;

	UFUNCTION()
	void OnRep_SwordRange();

	UPROPERTY(VisibleDefaultsOnly, Category = "Detection")
	class UDecalComponent* GroundDecalComponent;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	class UParticleSystemComponent* EffectVFX;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	class UParticleSystemComponent* PlasmaVFX;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	class UParticleSystemComponent* ImpactVFX;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	class UParticleSystemComponent* LandingVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<class UGameplayEffect> BindandDisarmEffect;

	UPROPERTY(VisibleDefaultsOnly, Category = "Component")
	class USceneComponent* RootComp;

	UPROPERTY(VisibleDefaultsOnly, Category = "Component")
	class USphereComponent* DetectionSphereComponent;

	UPROPERTY(VisibleDefaultsOnly, Category = "Component")
	class UStaticMeshComponent* SwordMeshComp;

	UFUNCTION()
	void OnRep_SwordLocation();

	UFUNCTION()
	void ActorInSwordRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void ActorLeftSwordRange(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void TryAddTarget(AActor* OtherTarget);
	void RemoveTarget(AActor* OtherTarget);

	TArray<AActor*> ActorsInRange;

	void StopSword();
	void BindTargets();
};
