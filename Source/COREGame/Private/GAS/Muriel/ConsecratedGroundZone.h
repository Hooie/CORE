// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenericTeamAgentInterface.h"
#include "ConsecratedGroundZone.generated.h"

UCLASS()
class AConsecratedGroundZone : public AActor, public IGenericTeamAgentInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AConsecratedGroundZone();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ConfigureZone(
		float InRadius,
		float InDuration,
		FGenericTeamId InTeamId,
		class UAbilitySystemComponent* InOwnerASC,
		TSubclassOf<class UGameplayEffect> InShieldEffect,
		TSubclassOf<class UGameplayEffect> InShieldBuffIconEffect,
		float InShieldDuration,
		TSubclassOf<class UGameplayEffect> InDamageEffect,
		float InBaseDamage,
		float InScalingRatio,
		int32 InAbilityLevel
	);

	void ApplyVisualScale();
	void DetonateEarly();

	void SetShieldAmount(float InShieldAmount) { ShieldAmount = InShieldAmount; };

	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;
	virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Component")
	class USphereComponent* TriggerVolume;

	UPROPERTY(VisibleAnywhere, Category = "Component")
	class UStaticMeshComponent* ZoneMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Detonate")
	float PushSpeed = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "Detonate")
	float PushUpwardRatio = 0.5f;

	UPROPERTY(ReplicatedUsing = OnRep_Radius)
	float Radius = 0.f;

	float Duration = 7.f;

	float ShieldAmount = -1.f;

	UPROPERTY(Replicated)
	FGenericTeamId TeamId;

	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	bool bShouldDrawDebug = false;

	UFUNCTION()
	void OnRep_Radius();

	UPROPERTY()
	class UAbilitySystemComponent* OwnerASC;

	TSubclassOf<class UGameplayEffect> ShieldEffect;

	TSubclassOf<class UGameplayEffect> ShieldBuffIconEffect;

	UPROPERTY()
	TSubclassOf<class UGameplayEffect> DamageEffect;

	float BaseDamage = 0.f;
	float ScalingRatio = 1.f;
	int32 AbilityLevel = 1;
	float ShieldDuration = 7.f;

	TSet<TWeakObjectPtr<AActor>> AlreadyShilededAllies;
	
	FTimerHandle NaturalExpiryTimerHandle;
	bool bDetonated = false;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void Detonate();

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	class UParticleSystem* SpawnVFX; 

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	class UParticleSystem* DetonationVFX;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayDetonationVFX();
};
