// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/MinionBarrack.h"
#include "AI/Minion.h"
#include "AI/LaneWayPointPath.h"
#include "Framework/CGameState.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "ProfilingDebugging/MiscTrace.h"

// Sets default values
AMinionBarrack::AMinionBarrack()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;
}

// Called when the game starts or when spawned
void AMinionBarrack::BeginPlay()
{
	Super::BeginPlay();
}

void AMinionBarrack::StartSpawning(float FirstSpawnDelay)
{
	if ( !HasAuthority() )
		return;

	GetWorldTimerManager().SetTimer(SpawnIntervalTimerHandle, this, &AMinionBarrack::SpawnNewGroup, GroupSpawnInterval, true, FMath::Max(0.f, FirstSpawnDelay));
}

void AMinionBarrack::StopSpawning()
{
	GetWorldTimerManager().ClearTimer(SpawnIntervalTimerHandle);
}

void AMinionBarrack::SpawnNewGroup()
{
	TRACE_BOOKMARK(TEXT("Wave: %s"), *GetName()); 
	
	if ( ACGameState* CGameState = GetWorld()->GetGameState<ACGameState>() )
	{
		CGameState->RegisterFirstWaveSpawnTime();
	}

	int32 SpotIndex = 0;

	SpawnMinionLine(MeleeMinionClass, MeleeMinionCount, SpotIndex, MeleeMinionPool);
	SpawnMinionLine(RangedMinionClass, RangedMinionCount, SpotIndex, RangedMinionPool);
}

void AMinionBarrack::SpawnMinionLine(TSubclassOf<AMinion> MinionClass, int32 Count, int32& InOutSpotIndex, TArray<AMinion*>& Pool)
{
	if ( Count <= 0 || !MinionClass )
		return;

	for ( int32 i = 0; i < Count; ++i )
	{
		FTransform SpawnTransform = GetActorTransform();

		if ( SpawnSpots.Num() > 0 )
		{
			if ( const APlayerStart* SpawnSpot = SpawnSpots[ InOutSpotIndex % SpawnSpots.Num() ] )
			{
				SpawnTransform = SpawnSpot->GetActorTransform();
			}
		}

		Multicast_PlaySpawnVFX(SpawnTransform.GetLocation());

		if ( AMinion* PooledMinion = GetNextAvailableMinion(Pool) )
		{
			// 풀에 쉬고 있는 미니언 재사용
			PooledMinion->SetActorTransform(SpawnTransform);
			PooledMinion->SetLane(WaypointPath, bReverseWaypointPath);
			PooledMinion->Activate();
		}
		else
		{
			AMinion* NewMinion = GetWorld()->SpawnActorDeferred<AMinion>(MinionClass, SpawnTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
			NewMinion->SetGenericTeamId(BarrackTeamId);
			NewMinion->FinishSpawning(SpawnTransform);
			NewMinion->SetLane(WaypointPath, bReverseWaypointPath);
			Pool.Add(NewMinion);
		}

		++InOutSpotIndex;
	}
}

AMinion* AMinionBarrack::GetNextAvailableMinion(TArray<AMinion*>& Pool) const
{
	AMinion* const* Found = Pool.FindByPredicate([] (const AMinion* Minion) { return IsValid(Minion) && !Minion->IsActive(); });

	return Found ? *Found : nullptr;
}

void AMinionBarrack::Multicast_PlaySpawnVFX_Implementation(FVector SpawnLocation)
{
	if ( SpawnVFX )
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SpawnVFX, SpawnLocation, FRotator::ZeroRotator);
	}
}

