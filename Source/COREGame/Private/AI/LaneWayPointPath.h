// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaneWayPointPath.generated.h"

/**
 
 * 웨이포인트들을 이은 꺾은선 레인
 * 미니언은 몇번째 웨이포인트가 아니라 경로 시작점에서 몇 cm 진행했는지로 위치 관리
 * 거리 계산은 Z를 무시한 2D 기준
	
**/

UCLASS()
class ALaneWayPointPath : public AActor
{
	GENERATED_BODY()
	
public:
	ALaneWayPointPath();

	// 경로 전체 길이 (cm)
	float GetTotalLength() const;

	// 경로 시작점에서 Distance만큼 진행한 경로 위의 위치
	FVector GetLocationAtDistance(float Distance) const;

	// Location을 경로에 투영했을 때의 진행 거리. [MinDistance, MaxDistance] 구간과 겹치는 선분만 검사 (음수면 전체)
	float ProjectToPath(const FVector& Location, float MinDistance = -1.f, float MaxDistance = -1.f) const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditAnywhere, Category = "Waypoint")
	TArray<class ALaneWaypoint*> Waypoints;

	// 게임 중 경로를 선으로 표시 (확인용)
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bDrawDebugPath = false;

	// 비어 있는(None) 슬롯과 겹친 점을 제외한 실제 경로 점, 각 점까지의 누적 거리
	mutable TArray<FVector> CachedPoints;
	mutable TArray<float> CumulativeDistances;
	mutable bool bCacheBuilt = false;

	void EnsureCache() const;

	// Distance가 속한 선분 [i, i+1]의 i
	int32 FindSegmentIndex(float Distance) const;
};