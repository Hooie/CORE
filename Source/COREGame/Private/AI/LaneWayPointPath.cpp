// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/LaneWayPointPath.h"
#include "AI/LaneWaypoint.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"

ALaneWayPointPath::ALaneWayPointPath()
{
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root Comp"));
}

void ALaneWayPointPath::BeginPlay()
{
	Super::BeginPlay();

	EnsureCache();

	if ( bDrawDebugPath )
	{
		for ( int32 i = 0; i + 1 < CachedPoints.Num(); ++i )
		{
			DrawDebugLine(GetWorld(), CachedPoints[ i ], CachedPoints[ i + 1 ], FColor::Cyan, true, -1.f, 0, 8.f);
		}
	}
}

float ALaneWayPointPath::GetTotalLength() const
{
	EnsureCache();

	return CumulativeDistances.Num() > 0 ? CumulativeDistances.Last() : 0.f;
}

FVector ALaneWayPointPath::GetLocationAtDistance(float Distance) const
{
	EnsureCache();

	const int32 Num = CachedPoints.Num();
	if ( Num == 0 )
		return GetActorLocation();

	if ( Num == 1 )
		return CachedPoints[ 0 ];

	Distance = FMath::Clamp(Distance, 0.f, CumulativeDistances.Last());

	const int32 i = FindSegmentIndex(Distance);
	const float SegmentLength = CumulativeDistances[ i + 1 ] - CumulativeDistances[ i ];
	const float Alpha = SegmentLength > 0.f ? (Distance - CumulativeDistances[ i ]) / SegmentLength : 0.f;

	return FMath::Lerp(CachedPoints[ i ], CachedPoints[ i + 1 ], Alpha);
}

float ALaneWayPointPath::ProjectToPath(const FVector& Location, float MinDistance, float MaxDistance) const
{
	EnsureCache();

	const int32 Num = CachedPoints.Num();
	if ( Num < 2 )
		return 0.f;

	const float Total = CumulativeDistances.Last();
	const float RangeMin = MinDistance < 0.f ? 0.f : MinDistance;
	const float RangeMax = MaxDistance < 0.f ? Total : MaxDistance;

	const FVector2D Location2D(Location.X, Location.Y);

	float BestDistance = RangeMin;
	float BestDistSq = TNumericLimits<float>::Max();

	for ( int32 i = 0; i + 1 < Num; ++i )
	{
		if ( CumulativeDistances[ i + 1 ] < RangeMin || CumulativeDistances[ i ] > RangeMax )
			continue;

		const FVector2D A(CachedPoints[ i ].X, CachedPoints[ i ].Y);
		const FVector2D B(CachedPoints[ i + 1 ].X, CachedPoints[ i + 1 ].Y);
		const FVector2D AB = B - A;

		const float LengthSq = AB.SizeSquared();
		const float T = LengthSq > 0.f ? FMath::Clamp(FVector2D::DotProduct(Location2D - A, AB) / LengthSq, 0.f, 1.f) : 0.f;

		const float DistSq = FVector2D::DistSquared(Location2D, A + AB * T);
		if ( DistSq < BestDistSq )
		{
			BestDistSq = DistSq;
			BestDistance = CumulativeDistances[ i ] + T * (CumulativeDistances[ i + 1 ] - CumulativeDistances[ i ]);
		}
	}

	return FMath::Clamp(BestDistance, RangeMin, RangeMax);
}

void ALaneWayPointPath::EnsureCache() const
{
	if ( bCacheBuilt )
		return;

	bCacheBuilt = true;
	CachedPoints.Reset();
	CumulativeDistances.Reset();

	for ( const ALaneWaypoint* Waypoint : Waypoints )
	{
		if ( !IsValid(Waypoint) )
		{
			UE_LOG(LogTemp, Warning, TEXT("[Lane] %s : Waypoints 배열에 비어 있는 슬롯이 있습니다. 건너뜁니다."), *GetName());
			continue;
		}

		const FVector PointLocation = Waypoint->GetActorLocation();

		if ( CachedPoints.Num() == 0 )
		{
			CachedPoints.Add(PointLocation);
			CumulativeDistances.Add(0.f);
			continue;
		}

		const float SegmentLength = FVector::Dist2D(CachedPoints.Last(), PointLocation);
		if ( SegmentLength < 1.f )
			continue;

		CachedPoints.Add(PointLocation);
		CumulativeDistances.Add(CumulativeDistances.Last() + SegmentLength);
	}
}

int32 ALaneWayPointPath::FindSegmentIndex(float Distance) const
{
	const int32 LastSegment = CachedPoints.Num() - 2;

	for ( int32 i = 0; i < LastSegment; ++i )
	{
		if ( Distance <= CumulativeDistances[ i + 1 ] )
			return i;
	}

	return FMath::Max(0, LastSegment);
}
