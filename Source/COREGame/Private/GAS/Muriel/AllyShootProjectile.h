// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "GenericTeamAgentInterface.h"
#include "AllyShootProjectile.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAllyHit, AActor*, HitActor, UAbilitySystemComponent*, HitASC);

UCLASS()
class AAllyShootProjectile : public AActor, public IGenericTeamAgentInterface
{
	GENERATED_BODY()
	
public:	
	FOnAllyHit OnAllyHit;

	AAllyShootProjectile();

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void ShootProjectile(
		float InSpeed,
		float InMaxDistance,
		const AActor* InTarget,
		FGenericTeamId InTeamId,
		FGameplayEffectSpecHandle InHitEffectHandle
	);

	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;
	virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Cue")
	FGameplayTag HitGameplayCueTag;

	UPROPERTY(VisibleAnywhere, Category = "Collision")
	class USphereComponent* CollisionComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Collision")
	float CollisionRadius = 50.f;

	UPROPERTY(VisibleAnywhere, Category = "Visual")
	class UParticleSystemComponent* ProjectileVFX;

	UPROPERTY(Replicated)
	FGenericTeamId TeamId;

	UPROPERTY(Replicated)
	FVector MoveDir;

	UPROPERTY(Replicated)
	float ProjectileSpeed;

	UPROPERTY()
	const AActor* Target;

	FGameplayEffectSpecHandle HitEffectSpecHandle;

	FTimerHandle ShootTimerHandle;

	void TravelMaxDistanceReached();

	void SendLocalGameplayCue(AActor* CueTargetActor, const FHitResult& HitResult);
};
