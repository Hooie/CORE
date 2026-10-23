// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "RangedMinionProjectile.generated.h"

UCLASS()
class ARangedMinionProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	ARangedMinionProjectile();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void Launch(AActor* InDesignatedTarget, float InSpeed, float InMaxDistance, FGameplayEffectSpecHandle InHitEffectHandle);

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Component")
	class UCapsuleComponent* CollisionComponent;

	UPROPERTY(VisibleAnywhere, Category = "Component")
	class UParticleSystemComponent* ProjectileVFX;

	UPROPERTY(VisibleAnywhere, Category = "Component")
	class UStaticMeshComponent* ProjectileMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Cue")
	FGameplayTag HitGameplayCueTag;

	UPROPERTY(Replicated)
	FVector MoveDir;

	UPROPERTY(Replicated)
	float ProjectileSpeed = 1000.f;

	UPROPERTY(Replicated) 
	TWeakObjectPtr<AActor> DesignatedTarget;

	FGameplayEffectSpecHandle HitEffectSpecHandle;
	FTimerHandle LifetimeTimerHandle;

	static const FRotator MeshOrientationOffset;

	void ExpireProjectile();

	void SendLocalGameplayCue(AActor* CueTargetActor, const FHitResult& HitResult);
};
