// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/CAIController.h"
#include "AI/AggroSubsystem.h"
#include "AI/Minion.h"
#include "AI/Turret/Turret.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Navigation/PathFollowingComponent.h"

#include "ProfilingDebugging/CpuProfilerTrace.h"

ACAIController::ACAIController()
{
}

void ACAIController::OnPossess(APawn* NewPawn)
{
	Super::OnPossess(NewPawn);

	if ( IGenericTeamAgentInterface* PawnTeamInterface = Cast<IGenericTeamAgentInterface>(NewPawn) )
	{
		SetGenericTeamId(PawnTeamInterface->GetGenericTeamId());
	}

	if ( UAbilitySystemComponent* PawnASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(NewPawn) )
	{
		PawnASC->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetDeadStatusTag()).AddUObject(this, &ACAIController::PawnDeadTagUpdated);
		PawnASC->RegisterGameplayTagEvent(UCAbilitySystemStatics::GetStunStatusTag()).AddUObject(this, &ACAIController::PawnStunTagUpdated);
	}

	// 미니언이 아니면 기존처럼 Behavior Tree 실행
	if ( !NewPawn || !NewPawn->IsA<AMinion>() )
	{
		if ( BehaviorTree )
		{
			RunBehaviorTree(BehaviorTree);
		}
		return;
	}

	ConfigureCrowdAgent();

	if ( ACharacter* MinionCharacter = Cast<ACharacter>(NewPawn) )
	{
		MinionCharacter->bUseControllerRotationYaw = false;

		if ( UCharacterMovementComponent* MoveComp = MinionCharacter->GetCharacterMovement() )
		{
			MoveComp->bOrientRotationToMovement = false;
			MoveComp->bUseControllerDesiredRotation = true;
		}
	}

	UBlackboardData* BlackboardAsset = MinionBlackboard;
	if ( !BlackboardAsset && BehaviorTree )
	{
		BlackboardAsset = BehaviorTree->BlackboardAsset;
	}

	if ( BlackboardAsset )
	{
		UBlackboardComponent* BlackboardComp = nullptr;
		UseBlackboard(BlackboardAsset, BlackboardComp);
	}

	// 판단 시점이 같은 프레임에 몰리지 않도록 시작 시간을 랜덤하게
	GetWorldTimerManager().SetTimer(ThinkTimerHandle, this, &ACAIController::MinionThink, ThinkInterval, true, FMath::FRandRange(0.f, ThinkInterval));
}

void ACAIController::OnUnPossess()
{
	GetWorldTimerManager().ClearTimer(ThinkTimerHandle);

	Super::OnUnPossess();
}

void ACAIController::StopAI()
{
	bAIStopped = true;

	GetWorldTimerManager().ClearTimer(ThinkTimerHandle);
	HaltPawn();
	SetBrainLogicEnabled(false, TEXT("AI Stopped"));
}

void ACAIController::PawnDeadTagUpdated(const FGameplayTag Tag, int32 Count)
{
	bIsPawnDead = Count != 0;

	if ( bIsPawnDead )
	{
		HaltPawn();

		// 시체가 다른 미니언의 회피 대상으로 남지 않도록 Crowd에서 제거
		SetCrowdAgentEnabled(false);
		SetBrainLogicEnabled(false, TEXT("Dead"));
	}
	else
	{
		SetCrowdAgentEnabled(true);

		if ( !bAIStopped )
		{
			SetBrainLogicEnabled(true);
		}
	}
}

void ACAIController::PawnStunTagUpdated(const FGameplayTag Tag, int32 Count)
{
	bIsPawnStunned = Count != 0;

	if ( bIsPawnDead || bAIStopped )
		return;

	if ( bIsPawnStunned )
	{
		StopMovement();
	}

	SetBrainLogicEnabled(!bIsPawnStunned, TEXT("Stun"));
}

void ACAIController::HaltPawn()
{
	StopMovement();
	SetCurrentTarget(nullptr, EMinionTargetPriority::None);
	SetFacingTarget(nullptr);
}

void ACAIController::SetBrainLogicEnabled(bool bEnabled, const FString& Reason)
{
	// 미니언은 BT를 실행하지 않으므로 BrainComponent가 없음 (BT 폰에서만 동작)
	UBrainComponent* Brain = GetBrainComponent();
	if ( !Brain )
		return;

	if ( bEnabled )
	{
		Brain->StartLogic();
	}
	else
	{
		Brain->StopLogic(Reason);
	}
}

void ACAIController::MinionThink()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Core_MinionThink);

	AMinion* Minion = GetMinion();
	if ( !Minion || bAIStopped || bIsPawnDead )
		return;

	if ( bIsPawnStunned )
	{
		StopMovement();
		return;
	}

	// 0) 우회 결과 (성공: 막 도착 / 실패: 시간 초과·갇힘)
	const bool bDetourArrived = ConsumeDetourArrival();
	const bool bDetourFailed = bDetourFailedPending;
	bDetourFailedPending = false;

	// 1) 현재 대상 유지 여부
	if ( CurrentTarget.IsValid() && !ShouldKeepCurrentTarget() )
	{
		SetCurrentTarget(nullptr, EMinionTargetPriority::None);
	}

	if ( CurrentTarget.IsValid() )
	{
		CurrentTargetPriority = EvaluatePriority(CurrentTarget.Get());
	}

	// 2) 대상 선택
	if ( bDetourArrived || bDetourFailed )
	{
		// 우회가 끝난 위치에서 다시 고름. 실패했으면 지금 대상은 닿기 어려우므로 빼고 찾음
		AActor* FailedTarget = bDetourFailed ? CurrentTarget.Get() : nullptr;

		if ( SwitchTarget(FailedTarget, FailedTarget) )
		{
			// 추격 거리 제한 기준점도 지금 위치로
			EngageOrigin = Minion->GetActorLocation();
		}
		else if ( bDetourArrived )
		{
			// 같은 대상이면 자리만 지금 위치 기준으로 다시 고름
			bHasApproachSlot = false;
		}
		// 실패했는데 다른 대상이 없으면 지금 대상 유지 → ResolveMoveGoal에서 반대쪽으로 우회
	}
	else
	{
		int32 BestPriority = EMinionTargetPriority::None;
		AActor* BestTarget = SelectBestTarget(BestPriority);

		if ( !CurrentTarget.IsValid() )
		{
			// 대상이 없으면 최우선 대상
			if ( BestTarget )
			{
				SetCurrentTarget(BestTarget, BestPriority);
			}
		}
		else if ( BestTarget && BestTarget != CurrentTarget.Get() && BestPriority <= EMinionTargetPriority::EnemyMinionAttackingAllyHero && BestPriority < CurrentTargetPriority )
		{
			// 이미 대상이 있으면 아군 영웅이 공격받는 경우에만 전환
			SetCurrentTarget(BestTarget, BestPriority);
		}
	}

	// 3) 행동 : 사거리 안이면 공격, 밖이면 접근, 대상이 없으면 레인을 따라 전진
	AActor* Target = CurrentTarget.Get();
	if ( !Target )
	{
		MoveAlongLane();
		return;
	}

	if ( IsInAttackRange(Target) )
	{
		// 공격 자세에 처음 들어갈 때만 정지 (매번 StopMovement 하면 떨림)
		if ( !bInAttackStance )
		{
			bInAttackStance = true;
			ClearDetour();
			StopMovement();
		}

		SetFacingTarget(Target);
		TryAttack();
	}
	else
	{
		if ( bInAttackStance )
		{
			ResetApproachState();
		}

		UpdateApproachSlot(Target);

		// 사거리 밖에서 멈춰 있으면(교착·도달 불가 자리) 자리를 바꾸고, 반복되면 대상을 바꿈
		if ( HandleApproachStall(Target) )
			return;

		// 대상 주변에 빈 자리가 계속 없으면 다른 대상으로 전환
		if ( NoFreeSlotSince >= 0.f && GetNow() - NoFreeSlotSince > NoSlotRetargetDelay )
		{
			if ( SwitchTarget(Target, Target) )
				return;

			// 다른 대상도 없으면 잠시 뒤 다시 시도
			NoFreeSlotSince = GetNow();
		}

		MoveTowards(GetApproachLocation(Target), 10.f, Target);
	}

	if ( bDrawDebug )
	{
		DrawDebugLine(GetWorld(), Minion->GetActorLocation(), Target->GetActorLocation(), FColor::Red, false, ThinkInterval + 0.05f, 0, 3.f);

		// 추가: 내가 고른 자리 + 이동 상태(0 Idle, 1 Waiting, 2 Paused, 3 Moving) + 속도
		DrawDebugSphere(GetWorld(), GetApproachLocation(Target), 20.f, 8, bHasApproachSlot ? FColor::Cyan : FColor::Orange, false, ThinkInterval + 0.05f);
		DrawDebugString(GetWorld(), Minion->GetActorLocation() + FVector(0.f, 0.f, 120.f),
			FString::Printf(TEXT("St=%d V=%.0f"), static_cast< int32 >(GetMoveStatus()), Minion->GetVelocity().Size2D()),
			nullptr, FColor::White, ThinkInterval + 0.05f);
	}
}

AActor* ACAIController::SelectBestTarget(int32& OutPriority, const AActor* ExcludeActor) const
{
	OutPriority = EMinionTargetPriority::None;

	const APawn* OwnerPawn = GetPawn();
	if ( !OwnerPawn )
		return nullptr;

	const FVector OwnerLocation = OwnerPawn->GetActorLocation();
	const float Now = GetNow();

	TArray<AActor*> Candidates;
	GatherNearbyPawns(OwnerLocation, AcquisitionRange, Candidates);

	AActor* BestTarget = nullptr;
	float BestEdgeDistance = TNumericLimits<float>::Max();

	for ( AActor* Candidate : Candidates )
	{
		if ( Candidate == ExcludeActor || !IsValidHostileTarget(Candidate) )
			continue;

		// 자리가 없어 포기했던 대상은 쿨다운 동안 최하 우선순위
		const bool bAbandoned = Candidate == AbandonedTarget.Get() && Now - AbandonedTime < AbandonedTargetCooldown;
		const int32 Priority = bAbandoned ? EMinionTargetPriority::Abandoned : EvaluatePriority(Candidate);

		const float EdgeDistance = FMath::Max(0.f, FVector::Dist2D(OwnerLocation, Candidate->GetActorLocation()) - GetCollisionRadius(Candidate));

		if ( Priority < OutPriority || (Priority == OutPriority && EdgeDistance < BestEdgeDistance) )
		{
			OutPriority = Priority;
			BestEdgeDistance = EdgeDistance;
			BestTarget = Candidate;
		}
	}

	return BestTarget;
}

int32 ACAIController::EvaluatePriority(AActor* Candidate) const
{
	const bool bIsHero = UCAbilitySystemStatics::IsHero(Candidate);
	const bool bIsStructure = IsStructure(Candidate);
	const bool bIsMinion = !bIsHero && !bIsStructure;

	// 도움 요청: 후보가 최근에 내 근처의 아군을 공격했는가
	if ( const UAggroSubsystem* AggroSubsystem = GetWorld()->GetSubsystem<UAggroSubsystem>() )
	{
		AActor* Victim = AggroSubsystem->GetRecentVictim(Candidate, CallForHelpMemory);

		const bool bVictimIsNearbyAlly = Victim && !UCAbilitySystemStatics::IsActorDead(Victim)
			&& GetTeamAttitudeTowards(*Victim) == ETeamAttitude::Friendly
			&& FVector::DistSquared2D(GetPawn()->GetActorLocation(), Victim->GetActorLocation()) <= FMath::Square(CallForHelpRange);

		if ( bVictimIsNearbyAlly )
		{
			if ( UCAbilitySystemStatics::IsHero(Victim) )
			{
				if ( bIsHero )		return EMinionTargetPriority::EnemyHeroAttackingAllyHero;
				if ( bIsMinion )	return EMinionTargetPriority::EnemyMinionAttackingAllyHero;
			}
			else if ( !IsStructure(Victim) )
			{
				if ( bIsMinion )	return EMinionTargetPriority::EnemyMinionAttackingAllyMinion;
				if ( bIsStructure )	return EMinionTargetPriority::EnemyStructureAttackingAllyMinion;
				if ( bIsHero )		return EMinionTargetPriority::EnemyHeroAttackingAllyMinion;
			}
		}
	}

	if ( bIsMinion )	return EMinionTargetPriority::ClosestEnemyMinion;
	if ( bIsHero )		return EMinionTargetPriority::ClosestEnemyHero;

	return EMinionTargetPriority::ClosestEnemyStructure;
}

bool ACAIController::IsValidHostileTarget(AActor* Candidate) const
{
	if ( !IsValid(Candidate) || Candidate == GetPawn() || Candidate->IsHidden() )
		return false;

	if ( UCAbilitySystemStatics::IsActorDead(Candidate) )
		return false;

	if ( !UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Candidate) )
		return false;

	return GetTeamAttitudeTowards(*Candidate) == ETeamAttitude::Hostile;
}

bool ACAIController::ShouldKeepCurrentTarget() const
{
	AActor* Target = CurrentTarget.Get();
	const APawn* OwnerPawn = GetPawn();

	if ( !OwnerPawn || !IsValidHostileTarget(Target) )
		return false;

	const float TargetRadius = GetCollisionRadius(Target);

	if ( FVector::Dist2D(OwnerPawn->GetActorLocation(), Target->GetActorLocation()) > LoseTargetRange + TargetRadius )
		return false;

	if ( FVector::Dist2D(EngageOrigin, Target->GetActorLocation()) > ChaseLeashRange + TargetRadius )
		return false;

	return true;
}

void ACAIController::SetCurrentTarget(AActor* NewTarget, int32 Priority)
{
	if ( NewTarget != CurrentTarget.Get() )
	{
		ResetApproachState();
	}

	if ( NewTarget && !CurrentTarget.IsValid() && GetPawn() )
	{
		EngageOrigin = GetPawn()->GetActorLocation();
	}

	CurrentTarget = NewTarget;
	CurrentTargetPriority = NewTarget ? Priority : EMinionTargetPriority::None;

	if ( UBlackboardComponent* BlackboardComp = GetBlackboardComponent() )
	{
		if ( NewTarget )
		{
			BlackboardComp->SetValueAsObject(TargetBlackBoardKeyName, NewTarget);
		}
		else
		{
			BlackboardComp->ClearValue(TargetBlackBoardKeyName);
		}
	}
}

bool ACAIController::SwitchTarget(const AActor* ExcludeActor, AActor* AbandonActor)
{
	int32 NewPriority = EMinionTargetPriority::None;
	AActor* NewTarget = SelectBestTarget(NewPriority, ExcludeActor);

	if ( !NewTarget || NewTarget == CurrentTarget.Get() )
		return false;

	// 포기한 대상을 기억해서 곧바로 다시 돌아오지 않게
	if ( AbandonActor )
	{
		AbandonedTarget = AbandonActor;
		AbandonedTime = GetNow();
	}

	SetCurrentTarget(NewTarget, NewPriority);
	return true;
}

bool ACAIController::IsInAttackRange(AActor* Target) const
{
	const AMinion* Minion = GetMinion();
	if ( !Minion || !Target )
		return false;

	const float EdgeDistance = FVector::Dist2D(Minion->GetActorLocation(), Target->GetActorLocation()) - GetCollisionRadius(Minion) - GetCollisionRadius(Target);

	const float AllowedRange = Minion->GetAttackRange() + (bInAttackStance ? AttackRangeHysteresis : 0.f);

	return EdgeDistance <= AllowedRange;
}

void ACAIController::SetFacingTarget(AActor* Target)
{
	if ( Target )
	{
		SetFocus(Target);
	}
	else
	{
		ClearFocus(EAIFocusPriority::Gameplay);
	}
}

void ACAIController::TryAttack()
{
	AMinion* Minion = GetMinion();
	if ( !Minion )
		return;

	const float Now = GetNow();
	if ( Now - LastAttackTime < Minion->GetAttackInterval() )
		return;

	if ( UAbilitySystemComponent* ASC = Minion->GetAbilitySystemComponent() )
	{
		LastAttackTime = Now;
		ASC->PressInputID(static_cast< int32 >(Minion->GetAttackInputID()));
	}
}

FVector ACAIController::GetApproachLocation(AActor* Target) const
{
	if ( !Target )
		return FVector::ZeroVector;

	return Target->GetActorLocation() + FRotator(0.f, ApproachSlotYaw, 0.f).Vector() * GetApproachRingRadius(Target);
}

float ACAIController::GetApproachRingRadius(AActor* Target) const
{
	const AMinion* Minion = GetMinion();
	if ( !Minion || !Target )
		return 0.f;

	return GetCollisionRadius(Target) + GetCollisionRadius(Minion) + Minion->GetAttackRange() * 0.8f;
}

void ACAIController::UpdateApproachSlot(AActor* Target)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Core_UpdateApproachSlot);

	const APawn* OwnerPawn = GetPawn();
	if ( !OwnerPawn || !Target )
		return;

	const FVector TargetLocation = Target->GetActorLocation();
	const float RingRadius = GetApproachRingRadius(Target);
	const float MyRadius = GetCollisionRadius(OwnerPawn);
	const float Now = GetNow();

	// 대상 주변에 '멈춰 서 있는' 유닛 (나와 대상 제외). 지나가는 유닛은 자리를 차지한 것으로 보지 않음
	TArray<AActor*> Occupants;
	GatherNearbyPawns(TargetLocation, RingRadius + MyRadius + 100.f, Occupants);
	Occupants.RemoveAll([ this, Target ] (const AActor* Other) { return Other == Target || !IsBlockingUnit(Other, false); });

	auto IsSlotFree = [ & ] (float Yaw)
		{
			const FVector SlotLocation = TargetLocation + FRotator(0.f, Yaw, 0.f).Vector() * RingRadius;

			return !Occupants.ContainsByPredicate([ & ] (const AActor* Occupant)
				{
					return FVector::DistSquared2D(SlotLocation, Occupant->GetActorLocation()) < FMath::Square(GetCollisionRadius(Occupant) + MyRadius);
				});
		};

	// 1) 고른 지 얼마 안 됐으면 무조건 유지, 그 뒤에는 비어 있는 동안 유지 (자주 바꾸면 우왕좌왕함)
	if ( bHasApproachSlot && (Now - SlotChosenTime < SlotCommitTime || IsSlotFree(ApproachSlotYaw)) )
	{
		NoFreeSlotSince = -1.f;
		return;
	}

	// 2) 내 방향에서 가까운 순서(0, +1, -1, +2, -2 ...)로 빈 자리 탐색
	FVector FromTarget = OwnerPawn->GetActorLocation() - TargetLocation;
	FromTarget.Z = 0.f;

	const float BaseYaw = FromTarget.IsNearlyZero() ? OwnerPawn->GetActorRotation().Yaw + 180.f : FromTarget.Rotation().Yaw;

	// 자리 간격 = 내 지름 + 여유 간격을 원호 각도로 환산
	const float StepYaw = FMath::Max(15.f, FMath::RadiansToDegrees((MyRadius * 2.f + ApproachSlotSpacing) / FMath::Max(RingRadius, 1.f)));
	const int32 MaxStep = FMath::FloorToInt(MaxSlotSearchAngle / StepYaw);

	// 방금 정체로 포기한 자리는 잠시 후보에서 제외
	const bool bHasBlockedSlot = Now - BlockedSlotTime < BlockedSlotMemory;

	auto TryChooseSlot = [ & ] (float Yaw)
		{
			if ( bHasBlockedSlot && FMath::Abs(FMath::FindDeltaAngleDegrees(Yaw, BlockedSlotYaw)) < StepYaw * 0.5f )
				return false;

			if ( !IsSlotFree(Yaw) )
				return false;

			bHasApproachSlot = true;
			ApproachSlotYaw = Yaw;
			SlotChosenTime = Now;
			NoFreeSlotSince = -1.f;
			return true;
		};

	for ( int32 Step = 0; Step <= MaxStep; ++Step )
	{
		const float Offset = Step * StepYaw;

		if ( TryChooseSlot(BaseYaw + Offset) || (Step > 0 && TryChooseSlot(BaseYaw - Offset)) )
			return;
	}

	// 3) 빈 자리 없음 → 내 방향 자리로 다가가 대기, 오래 지속되면 MinionThink에서 다른 대상으로 전환
	bHasApproachSlot = false;
	ApproachSlotYaw = BaseYaw;

	if ( NoFreeSlotSince < 0.f )
	{
		NoFreeSlotSince = Now;
	}
}

void ACAIController::ResetApproachState()
{
	bInAttackStance = false;
	bHasApproachSlot = false;
	NoFreeSlotSince = -1.f;

	ClearDetour();
	FailedDetourSide = 0;
	bDetourFailedPending = false;

	LastStuckCheckTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	LastStuckCheckLocation = GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector;

	ApproachMovingTime = GetNow();
	ApproachStallCount = 0;
}

bool ACAIController::HandleApproachStall(AActor* Target)
{
	const APawn* OwnerPawn = GetPawn();
	if ( !OwnerPawn || !Target )
		return false;

	const float Now = GetNow();

	// 움직이고 있으면 정상 접근 중
	if ( OwnerPawn->GetVelocity().SizeSquared2D() > FMath::Square(StationarySpeedThreshold) )
	{
		ApproachMovingTime = Now;
		return false;
	}

	if ( Now - ApproachMovingTime < ApproachStallTime )
		return false;

	// 정체: 지금 자리는 잠시 후보에서 빼고, 새 자리로 경로를 다시 요청
	ApproachMovingTime = Now;
	++ApproachStallCount;

	BlockedSlotYaw = ApproachSlotYaw;
	BlockedSlotTime = Now;
	bHasApproachSlot = false;
	bForceRepath = true;

	if ( ApproachStallCount >= MaxApproachStallCount )
	{
		ApproachStallCount = 0;

		if ( SwitchTarget(Target, Target) )
			return true;
	}

	UpdateApproachSlot(Target);
	return false;
}

void ACAIController::MoveAlongLane()
{
	AMinion* Minion = GetMinion();
	if ( !Minion )
		return;

	FVector LaneGoal;
	if ( !Minion->GetLaneMoveGoal(LaneGoal) )
	{
		StopMovement();
		return;
	}

	MoveTowards(LaneGoal, LaneMoveAcceptanceRadius, nullptr);

	if ( bDrawDebug )
	{
		DrawDebugSphere(GetWorld(), LaneGoal, 30.f, 8, FColor::Green, false, ThinkInterval + 0.05f);
	}
}

void ACAIController::MoveTowards(const FVector& Goal, float AcceptanceRadius, const AActor* IgnoreActor)
{
	const FVector MoveGoal = ResolveMoveGoal(Goal, IgnoreActor);

	FaceMoveDirection(MoveGoal);
	UpdateMoveGoal(MoveGoal, bHasDetourPoint ? DetourReachRadius * 0.5f : AcceptanceRadius);

	if ( bDrawDebug && bHasDetourPoint )
	{
		DrawDebugSphere(GetWorld(), DetourPoint, 25.f, 8, FColor::Yellow, false, ThinkInterval + 0.05f);
	}
}

void ACAIController::UpdateMoveGoal(const FVector& Goal, float AcceptanceRadius)
{
	// 이동 중에 목표가 조금만 바뀌었으면 경로를 다시 요청하지 않음 (매번 요청하면 끊김)
	const bool bIdle = GetMoveStatus() == EPathFollowingStatus::Idle;

	if ( !bForceRepath && !bIdle && FVector::DistSquared2D(Goal, LastMoveGoal) < FMath::Square(MoveGoalUpdateThreshold) )
		return;

	bForceRepath = false;
	LastMoveGoal = Goal;

	MoveToLocation(Goal, AcceptanceRadius, /*bStopOnOverlap*/ false, /*bUsePathfinding*/ true, /*bProjectDestinationToNavigation*/ true, /*bCanStrafe*/ false, /*FilterClass*/ nullptr, /*bAllowPartialPath*/ true);
}

void ACAIController::FaceMoveDirection(const FVector& MoveGoal)
{
	const APawn* OwnerPawn = GetPawn();
	if ( !OwnerPawn )
		return;

	const FVector PawnLocation = OwnerPawn->GetActorLocation();

	// 목표 바로 앞에서는 방향이 뒤집히며 제자리에서 돌 수 있으므로 현재 방향 유지
	if ( FVector::DistSquared2D(PawnLocation, MoveGoal) < FMath::Square(FacingMinDistance) )
	{
		ClearFocus(EAIFocusPriority::Gameplay);
		return;
	}

	SetFocalPoint(FVector(MoveGoal.X, MoveGoal.Y, PawnLocation.Z));
}

FVector ACAIController::ResolveMoveGoal(const FVector& Goal, const AActor* IgnoreActor)
{
	if ( !GetPawn() )
		return Goal;

	const float Now = GetNow();
	const bool bStuck = CheckStuck();

	// 진행 중인 우회는 도착 / 시간 초과 / 갇힘 전까지 유지 (판단마다 좌우가 바뀌면 오히려 떨림)
	if ( bHasDetourPoint )
	{
		const bool bReached = IsDetourPointReached();
		const bool bTimedOut = Now - DetourStartTime > DetourTimeout;

		if ( !bReached && !bTimedOut && !bStuck )
			return DetourPoint;

		// 도착하지 못하고 끝났으면 실패 → 다음엔 반대쪽으로, 다음 판단에서 대상부터 다시 고름
		FailedDetourSide = bReached ? 0 : DetourSide;

		if ( !bReached )
		{
			bDetourFailedPending = true;
		}

		ClearDetour();
	}

	// 평소엔 멈춰 있는 유닛만, 실제로 갇혔으면 움직이는 유닛까지 포함해서 우회 지점 계산
	FVector NewDetourPoint;
	int32 NewDetourSide = 0;

	if ( !FindDetourPoint(Goal, IgnoreActor, bStuck, FailedDetourSide, NewDetourPoint, NewDetourSide) )
		return Goal;

	bHasDetourPoint = true;
	DetourPoint = NewDetourPoint;
	DetourSide = NewDetourSide;
	DetourStartTime = Now;

	return DetourPoint;
}

bool ACAIController::FindDetourPoint(const FVector& Goal, const AActor* IgnoreActor, bool bIncludeMovingUnits, int32 AvoidSide, FVector& OutPoint, int32& OutSide) const
{
	const APawn* OwnerPawn = GetPawn();
	if ( !OwnerPawn )
		return false;

	const FVector MyLocation = OwnerPawn->GetActorLocation();

	FVector Forward = Goal - MyLocation;
	Forward.Z = 0.f;

	// 목표 근처에서는 우회하지 않음 (마지막 접근은 Crowd 회피에 맡김)
	const float GoalDistance = Forward.Size();
	if ( GoalDistance < FMath::Max(1.f, DetourMinGoalDistance) )
		return false;

	Forward /= GoalDistance;
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);

	const float MyRadius = GetCollisionRadius(OwnerPawn);
	const float CheckDistance = FMath::Min(GoalDistance, BlockCheckDistance);

	// 진행 방향 앞쪽 + 옆으로 늘어선 유닛까지 포함하도록 넉넉하게 검사
	TArray<AActor*> Nearby;
	GatherNearbyPawns(MyLocation + Forward * (CheckDistance * 0.5f), CheckDistance * 0.5f + 300.f, Nearby);

	// 진행 방향 기준 좌표: Ahead = 이 유닛을 완전히 지나치는 앞쪽 거리, [Min, Max] = 피하려면 비워야 하는 가로 구간
	struct FBlockSpan
	{
		float Ahead;
		float Min;
		float Max;
	};

	TArray<FBlockSpan> Spans;

	for ( const AActor* Other : Nearby )
	{
		if ( Other == IgnoreActor || !IsBlockingUnit(Other, bIncludeMovingUnits) )
			continue;

		const FVector ToOther = Other->GetActorLocation() - MyLocation;
		const float AheadDistance = FVector::DotProduct(ToOther, Forward);
		const float OtherRadius = GetCollisionRadius(Other);

		// 옆에 붙어 있어도 아직 다 지나치지 못했으면 장애물로 취급
		const float PassAhead = AheadDistance + OtherRadius + MyRadius;
		if ( PassAhead <= 0.f || AheadDistance > CheckDistance )
			continue;

		const float Lateral = FVector::DotProduct(ToOther, Right);
		const float HalfWidth = OtherRadius + MyRadius + DetourClearance;

		Spans.Add(FBlockSpan{ PassAhead, Lateral - HalfWidth, Lateral + HalfWidth });
	}

	// 진행 경로(가로 0)를 막는 유닛이 없으면 우회 불필요
	const bool bBlocked = Spans.ContainsByPredicate([] (const FBlockSpan& Span) { return Span.Min < 0.f && 0.f < Span.Max; });
	if ( !bBlocked )
		return false;

	// 가로 0에서 시작해 겹쳐 있는 유닛들을 이어서 넘어가며, 완전히 빠져나갈 수 있는 가로 위치를 찾음
	auto FindClearLateral = [ &Spans ] (bool bRightSide, float& OutLateral, float& OutAhead)
		{
			float Lateral = 0.f;
			OutAhead = 0.f;

			bool bMoved = true;
			while ( bMoved )
			{
				bMoved = false;

				for ( const FBlockSpan& Span : Spans )
				{
					if ( Span.Min < Lateral && Lateral < Span.Max )
					{
						Lateral = bRightSide ? Span.Max : Span.Min;
						OutAhead = FMath::Max(OutAhead, Span.Ahead);
						bMoved = true;
					}
				}
			}

			OutLateral = Lateral;
		};

	float RightLateral, RightAhead, LeftLateral, LeftAhead;
	FindClearLateral(true, RightLateral, RightAhead);
	FindClearLateral(false, LeftLateral, LeftAhead);

	// 직전에 실패한 방향이 있으면 반대쪽, 없으면 더 짧게 돌아가는 쪽
	const bool bUseRight = AvoidSide != 0 ? AvoidSide < 0 : RightLateral <= -LeftLateral;

	const float Lateral = bUseRight ? RightLateral : LeftLateral;
	const float Ahead = FMath::Min((bUseRight ? RightAhead : LeftAhead) + DetourPassMargin, GoalDistance);

	// 막고 있는 유닛들을 완전히 지나친 뒤 + 여유 거리 지점
	OutPoint = MyLocation + Forward * Ahead + Right * Lateral;
	OutSide = bUseRight ? 1 : -1;

	// 우회 지점이 도착 판정 거리 안이면 의미 없음 → 목표로 직진
	return FVector::DistSquared2D(OutPoint, MyLocation) > FMath::Square(DetourReachRadius);
}

bool ACAIController::IsDetourPointReached() const
{
	const APawn* OwnerPawn = GetPawn();
	return OwnerPawn && FVector::DistSquared2D(OwnerPawn->GetActorLocation(), DetourPoint) <= FMath::Square(DetourReachRadius);
}

bool ACAIController::ConsumeDetourArrival()
{
	if ( !bHasDetourPoint || !IsDetourPointReached() )
		return false;

	ClearDetour();
	FailedDetourSide = 0;

	return true;
}

bool ACAIController::CheckStuck()
{
	const APawn* OwnerPawn = GetPawn();
	if ( !OwnerPawn )
		return false;

	const float Now = GetNow();
	if ( Now - LastStuckCheckTime < StuckCheckInterval )
		return false;

	const FVector Location = OwnerPawn->GetActorLocation();
	const bool bStuck = FVector::DistSquared2D(Location, LastStuckCheckLocation) < FMath::Square(StuckMinMove);

	LastStuckCheckTime = Now;
	LastStuckCheckLocation = Location;

	return bStuck;
}

UCrowdFollowingComponent* ACAIController::GetCrowdComponent() const
{
	return Cast<UCrowdFollowingComponent>(GetPathFollowingComponent());
}

void ACAIController::ConfigureCrowdAgent()
{
	UCrowdFollowingComponent* CrowdComp = GetCrowdComponent();
	if ( !CrowdComp )
	{
		UE_LOG(LogTemp, Warning, TEXT("[MinionAI] %s : PathFollowingComponent가 CrowdFollowingComponent가 아닙니다."), *GetName());
		return;
	}

	CrowdComp->SetCrowdSimulationState(ECrowdSimulationState::Enabled);
	CrowdComp->SetCrowdAvoidanceQuality(ECrowdAvoidanceQuality::High);
	CrowdComp->SetCrowdObstacleAvoidance(true);
	CrowdComp->SetCrowdSeparation(false);		
	CrowdComp->SetCrowdCollisionQueryRange(CrowdCollisionQueryRange);
	CrowdComp->SetCrowdAnticipateTurns(true);
	CrowdComp->SetCrowdOptimizeVisibility(true);
	CrowdComp->SetCrowdOptimizeTopology(true);
}

void ACAIController::SetCrowdAgentEnabled(bool bEnabled)
{
	UCrowdFollowingComponent* CrowdComp = GetCrowdComponent();
	if ( !CrowdComp )
		return;

	StopMovement();

	CrowdComp->SetCrowdSimulationState(bEnabled ? ECrowdSimulationState::Enabled : ECrowdSimulationState::Disabled);
}

AMinion* ACAIController::GetMinion() const
{
	return Cast<AMinion>(GetPawn());
}

float ACAIController::GetNow() const
{
	return GetWorld()->GetTimeSeconds();
}

void ACAIController::GatherNearbyPawns(const FVector& Center, float Radius, TArray<AActor*>& OutActors) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Core_GatherNearbyPawns);
	
	OutActors.Reset();

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MinionPawnScan), false, GetPawn());

	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Radius), Params);

	// 한 액터의 여러 컴포넌트가 겹칠 수 있으므로 중복 제거
	for ( const FOverlapResult& Overlap : Overlaps )
	{
		if ( AActor* Actor = Overlap.GetActor() )
		{
			OutActors.AddUnique(Actor);
		}
	}
}

bool ACAIController::IsBlockingUnit(const AActor* Actor, bool bIncludeMovingUnits) const
{
	if ( !Actor || IsStructure(Actor) || UCAbilitySystemStatics::IsActorDead(Actor) )
		return false;

	return bIncludeMovingUnits || Actor->GetVelocity().SizeSquared2D() <= FMath::Square(StationarySpeedThreshold);
}

float ACAIController::GetCollisionRadius(const AActor* Actor)
{
	if ( !Actor )
		return 0.f;

	if ( const ACharacter* Character = Cast<ACharacter>(Actor) )
	{
		return Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
	}

	if ( const UCapsuleComponent* Capsule = Actor->FindComponentByClass<UCapsuleComponent>() )
	{
		return Capsule->GetScaledCapsuleRadius();
	}

	return 0.f;
}

bool ACAIController::IsStructure(const AActor* Actor)
{
	return Actor && Actor->IsA<ATurret>();
}