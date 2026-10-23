// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Minion.h"
#include "AI/LaneWayPointPath.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Framework/CGameState.h"
#include "GAS/CAbilitySystemComponent.h"
#include "GAS/CHeroAttributeSet.h"
#include "Framework/CoreBench.h"

AMinion::AMinion()
{
	PrimaryActorTick.bCanEverTick = false;

	HeroAttributeSet = CreateDefaultSubobject<UCHeroAttributeSet>("Hero Attribute Set");

	SetNetUpdateFrequency(20.f);
	SetMinNetUpdateFrequency(5.f);
}

void AMinion::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
	Super::SetGenericTeamId(NewTeamId);
	PickSkinBasedOnTeamID();
}

void AMinion::BeginPlay()
{
	Super::BeginPlay();

	static const bool bNoOptMinionAnim = CoreBench::IsOptOff(TEXT("NoOptMinionAnim"));

	if ( GetNetMode() == NM_DedicatedServer && !bNoOptMinionAnim )
	{
		GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickMontagesAndRefreshBonesWhenPlayingMontages;
	}

	IgnoreOtherMinionsWhenMoving();

	if ( HasAuthority() )
	{
		GetWorldTimerManager().SetTimer(ScalingTimerHandle, this, &AMinion::UpdateScaling, ScalingUpdateInterval, true, 0.f);
	}
}

bool AMinion::IsActive() const
{
	return !IsDead();
}

void AMinion::Activate()
{
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	RespawnImmediately();
}

void AMinion::PickSkinBasedOnTeamID()
{
	if ( USkeletalMesh** Skin = SkinMap.Find(GetGenericTeamId()) )
	{
		GetMesh()->SetSkeletalMesh(*Skin);
	}
}

void AMinion::OnRep_TeamID()
{
	Super::OnRep_TeamID();

	PickSkinBasedOnTeamID();
}

void AMinion::UpdateScaling()
{
	const ACGameState* CGameState = GetWorld()->GetGameState<ACGameState>();
	if ( !CGameState )
		return;

	if ( UCAbilitySystemComponent* ASC = Cast<UCAbilitySystemComponent>(GetAbilitySystemComponent()) )
	{
		ASC->ApplyMinionTimeScaling(ScalingConfig, CGameState->GetTimeSinceFirstWave());
	}
}

void AMinion::SetLane(ALaneWayPointPath* InPath, bool bInReverse)
{
	LanePath = InPath;
	bReverseLane = bInReverse;
	LaneProgress = 0.f;

	if ( !LanePath )
		return;

	LaneProgress = ToPathDistance(LanePath->ProjectToPath(GetActorLocation()), LanePath->GetTotalLength());
}

bool AMinion::GetLaneMoveGoal(FVector& OutGoal)
{
	if ( !LanePath )
		return false;

	const float Total = LanePath->GetTotalLength();
	if ( Total <= 0.f )
		return false;

	// 1) 현재 위치를 레인에 투영해서 진행도 갱신 (직전 진행도 주변 구간만 검사 → 가까이 지나가는 다른 구간으로 튀지 않음)
	const float SearchA = ToPathDistance(FMath::Max(0.f, LaneProgress - ProgressSearchBack), Total);
	const float SearchB = ToPathDistance(FMath::Min(Total, LaneProgress + ProgressSearchForward), Total);

	const float ProjectedPathDistance = LanePath->ProjectToPath(GetActorLocation(), FMath::Min(SearchA, SearchB), FMath::Max(SearchA, SearchB));
	LaneProgress = ToPathDistance(ProjectedPathDistance, Total);

	// 2) 진행도보다 LaneLookAhead 앞의 레인 지점을 목표로 (웨이포인트에서 멈추지 않음)
	const float GoalProgress = FMath::Min(Total, LaneProgress + LaneLookAhead);
	OutGoal = LanePath->GetLocationAtDistance(ToPathDistance(GoalProgress, Total));

	return true;
}

void AMinion::IgnoreOtherMinionsWhenMoving()
{
	UCapsuleComponent* MyCapsule = GetCapsuleComponent();
	if ( !MyCapsule )
		return;

	// 풀링이라 미니언이 파괴되지 않으므로, 새 미니언이 생길 때 기존 미니언들과 서로 한 번씩만 등록
	for ( TActorIterator<AMinion> It(GetWorld()); It; ++It )
	{
		AMinion* Other = *It;
		if ( !Other || Other == this )
			continue;

		MyCapsule->IgnoreActorWhenMoving(Other, true);

		if ( UCapsuleComponent* OtherCapsule = Other->GetCapsuleComponent() )
		{
			OtherCapsule->IgnoreActorWhenMoving(this, true);
		}
	}
}