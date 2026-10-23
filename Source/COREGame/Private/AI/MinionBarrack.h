// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenericTeamAgentInterface.h"
#include "MinionBarrack.generated.h"

UCLASS()
class AMinionBarrack : public AActor
{
	GENERATED_BODY()
	
public:
	AMinionBarrack();

	void StartSpawning(float FirstSpawnDelay);
	void StopSpawning();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "Spawn")
	FGenericTeamId BarrackTeamId;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	int32 MeleeMinionCount = 3;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	int32 RangedMinionCount = 3;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	float GroupSpawnInterval = 15.f;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	TSubclassOf<class AMinion> MeleeMinionClass;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	TSubclassOf<class AMinion> RangedMinionClass;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	bool bReverseWaypointPath = false;

	UPROPERTY()
	TArray<class AMinion*> MeleeMinionPool;

	UPROPERTY()
	TArray<class AMinion*> RangedMinionPool;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	class ALaneWayPointPath* WaypointPath;

	UPROPERTY(EditAnywhere, Category = "Spawn")
	TArray<class APlayerStart*> SpawnSpots;

	void SpawnNewGroup();
	void SpawnMinionLine(TSubclassOf<AMinion> MinionClass, int32 Count, int32& InOutSpotIndex, TArray<AMinion*>& Pool);

	AMinion* GetNextAvailableMinion(TArray<AMinion*>& Pool) const;

	FTimerHandle SpawnIntervalTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Spawn")
	class UParticleSystem* SpawnVFX;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlaySpawnVFX(FVector SpawnLocation);
};
