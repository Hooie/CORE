// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "GenericTeamAgentInterface.h"
#include "TA_StaticTrap.generated.h"

/**
 * 
 */
UCLASS()
class ATA_StaticTrap : public AGameplayAbilityTargetActor
{
	GENERATED_BODY()
	
public:
	ATA_StaticTrap();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ConfigureTrap(
		float InTriggerRadius,
		float InTrapLifetime,
		FGenericTeamId InTeamId,
		class UAbilitySystemComponent* InOwnerASC,
		TSubclassOf<class UGameplayEffect> InDamageEffect,
		float InBaseDamage,
		float InScalingRatio,
		int32 InAbilityLevel,
		TSubclassOf<class UGameplayEffect> InBindEffect
	);

	void SetTrapLocation(const FVector& Location);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleDefaultsOnly, Category = "Component")
	class USceneComponent* RootComp;

	UPROPERTY(VisibleDefaultsOnly, Category = "Component")
	class USphereComponent* TriggerVolume;

	UPROPERTY(EditAnywhere, Category = "Component")
	class USkeletalMeshComponent* TrapMesh;

	UPROPERTY(VisibleDefaultsOnly, Category = "Component")
	class UParticleSystemComponent* SettingTrapVFX_Smoke;
	
	UPROPERTY(VisibleDefaultsOnly, Category = "Component")
	class UParticleSystemComponent* SettingTrapVFX_Core;

	UPROPERTY(VisibleDefaultsOnly, Category = "Detection")
	class UDecalComponent* GroundDecalComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	class UParticleSystem* DeactivationVFX;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	class UParticleSystem* ActivationLoopVFX;

	UPROPERTY()
	class UParticleSystemComponent* ActivationLoopVFXComponent;

	float TrapLifetime = 30.f;

	UPROPERTY(Replicated)
	FGenericTeamId OwnerTeamId;

	UPROPERTY(ReplicatedUsing = OnRep_TriggerRadius) 
	float TriggerRadius = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_TrapLocation)
	FVector ReplicatedLocation = FVector::ZeroVector;

	UPROPERTY()
	class UAbilitySystemComponent* OwnerASC;

	UPROPERTY()
	TSubclassOf<class UGameplayEffect> DamageEffect;

	float BaseDamage = 0.f;
	float ScalingRatio = 0.f;
	int32 AbilityLevel = 1;

	UPROPERTY()
	TSubclassOf<class UGameplayEffect> BindEffect;

	bool bHasTriggered = false;
	TWeakObjectPtr<AActor> BoundTarget;
	FActiveGameplayEffectHandle BoundEffectHandle;

	FTimerHandle LifetimeTimerHandle;
	FTimerHandle BindReleaseTimerHandle;

	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void SelfDestructIfNotTriggered();
	void ReleaseBindAndDestroy();

	UFUNCTION()
	void OnRep_TriggerRadius();

	UFUNCTION()
	void OnRep_TrapLocation();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayActivationVFX();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayDeactivationVFX();
};
