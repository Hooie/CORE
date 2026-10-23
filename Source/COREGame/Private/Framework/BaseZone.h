// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameFramework/Actor.h"
#include "BaseZone.generated.h"

UCLASS()
class ABaseZone : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABaseZone();

	uint8 GetTeamId() const { return TeamId; }

	float GetTriggerVolumeHalfHeight();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "Team")
	uint8 TeamId = 0;

	UPROPERTY(VisibleAnywhere, Category = "Component")
	class UBoxComponent* TriggerBoxVolume;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<class UGameplayEffect> InBaseEffect;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	FTimerHandle InitialOverlapCheckTimerHandle;
	void CheckInitialOverlaps();

	TSet<TWeakObjectPtr<AActor>> ActorsInBase;

	TMap<TWeakObjectPtr<AActor>, FActiveGameplayEffectHandle> ActiveRegenEffects;
};
