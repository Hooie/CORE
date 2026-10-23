// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DetourCrowdAIController.h"
#include "GameplayTagContainer.h"
#include "CAIController.generated.h"

// 리그 오브 레전드 미니언 대상 우선순위 (숫자가 작을수록 우선)
namespace EMinionTargetPriority
{
	enum Type : int32
	{
		EnemyHeroAttackingAllyHero = 1,		// 1. 아군 영웅을 공격한 적 영웅
		EnemyMinionAttackingAllyHero,		// 2. 아군 영웅을 공격한 적 미니언
		EnemyMinionAttackingAllyMinion,		// 3. 아군 미니언을 공격한 적 미니언
		EnemyStructureAttackingAllyMinion,	// 4. 아군 미니언을 공격한 적 구조물
		EnemyHeroAttackingAllyMinion,		// 5. 아군 미니언을 공격한 적 영웅
		ClosestEnemyMinion,					// 6. 가장 가까운 적 미니언
		ClosestEnemyHero,					// 7. 가장 가까운 적 영웅
		ClosestEnemyStructure,				// 8. 가장 가까운 적 구조물
		Abandoned,							// 자리가 없어 포기한 대상 (다른 대상이 없을 때만)

		None = MAX_int32
	};
}

/**
 * 미니언에 빙의하면 C++ 레인 AI(이동/대상 선택/공격), 그 외 폰은 Behavior Tree 실행
 */
UCLASS()
class ACAIController : public ADetourCrowdAIController
{
	GENERATED_BODY()

public:
	ACAIController();

	virtual void OnPossess(APawn* NewPawn) override;
	virtual void OnUnPossess() override;

	// 경기 종료 등: AI 완전 정지
	void StopAI();

private:
	/************************************/
	/***************State****************/
	/************************************/
	void PawnDeadTagUpdated(const FGameplayTag Tag, int32 Count);
	void PawnStunTagUpdated(const FGameplayTag Tag, int32 Count);

	// 이동 정지 + 대상/포커스 해제
	void HaltPawn();
	void SetBrainLogicEnabled(bool bEnabled, const FString& Reason = FString());

	bool bIsPawnDead = false;
	bool bIsPawnStunned = false;
	bool bAIStopped = false;

	/************************************/
	/***************Think****************/
	/************************************/
	void MinionThink();

	FTimerHandle ThinkTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Minion AI")
	float ThinkInterval = 0.2f;

	/************************************/
	/**************Targeting*************/
	/************************************/
	AActor* SelectBestTarget(int32& OutPriority, const AActor* ExcludeActor = nullptr) const;
	int32 EvaluatePriority(AActor* Candidate) const;
	bool IsValidHostileTarget(AActor* Candidate) const;
	bool ShouldKeepCurrentTarget() const;
	void SetCurrentTarget(AActor* NewTarget, int32 Priority);

	// ExcludeActor를 뺀 최선의 대상으로 전환 (전환 시 AbandonActor는 쿨다운 동안 최하 우선순위). 전환했으면 true
	bool SwitchTarget(const AActor* ExcludeActor, AActor* AbandonActor);

	TWeakObjectPtr<AActor> CurrentTarget;
	int32 CurrentTargetPriority = EMinionTargetPriority::None;
	FVector EngageOrigin = FVector::ZeroVector;

	TWeakObjectPtr<AActor> AbandonedTarget;
	float AbandonedTime = -100.f;

	UPROPERTY(EditDefaultsOnly, Category = "AI Behavior")
	FName TargetBlackBoardKeyName = "Target";

	// 미니언이 아닌 폰에서만 실행. 미니언은 이 BT의 블랙보드만 사용 (원거리 공격 GA가 Target 키를 읽음)
	UPROPERTY(EditDefaultsOnly, Category = "AI Behavior")
	class UBehaviorTree* BehaviorTree;

	// 비워두면 BT의 블랙보드 사용
	UPROPERTY(EditDefaultsOnly, Category = "AI Behavior")
	class UBlackboardData* MinionBlackboard;

	// 새로운 타겟을 찾는 반경
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI")
	float AcquisitionRange = 900.f;

	// 타겟을 놓치는 거리
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI")
	float LoseTargetRange = 1300.f;

	// 교전 시작 후 쫓을 수 있는 최대 거리 (레인 이탈 방지)
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI")
	float ChaseLeashRange = 1000.f;

	// 이 반경 내의 아군이 공격받으면 도움 요청으로 처리
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI")
	float CallForHelpRange = 1000.f;

	// 누가 누구를 공격 중인지 기록을 인정하는 시간
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI")
	float CallForHelpMemory = 1.5f;

	// 자리가 없어 포기한 대상은 이 시간 동안 다른 대상이 없을 때만 다시 고름
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Approach")
	float AbandonedTargetCooldown = 3.f;

	/************************************/
	/***************Combat***************/
	/************************************/
	bool IsInAttackRange(AActor* Target) const;
	void SetFacingTarget(AActor* Target);
	void TryAttack();

	bool bInAttackStance = false;
	float LastAttackTime = -1000.f;

	// 공격 중일 때는 사거리 + 이 값까지 벗어나야 다시 이동 (경계에서 멈춤/이동 반복 방지)
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Approach")
	float AttackRangeHysteresis = 50.f;

	/************************************/
	/**************Approach**************/
	/************************************/
	FVector GetApproachLocation(AActor* Target) const;
	float GetApproachRingRadius(AActor* Target) const;
	void UpdateApproachSlot(AActor* Target);
	void ResetApproachState();

	bool bHasApproachSlot = false;
	float ApproachSlotYaw = 0.f;		// 대상 기준 자리 방향 (월드 Yaw)
	float SlotChosenTime = 0.f;
	float NoFreeSlotSince = -1.f;		// 빈 자리가 없기 시작한 시간 (-1 = 해당 없음)

	// 대상 주변 자리 사이의 여유 간격
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Approach")
	float ApproachSlotSpacing = 20.f;

	// 한 번 고른 자리는 최소 이 시간 동안 유지
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Approach")
	float SlotCommitTime = 1.f;

	// 내 방향 기준 좌우 이 각도 안에서만 빈 자리를 찾음
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Approach")
	float MaxSlotSearchAngle = 90.f;

	// 대상 주변에 빈 자리가 없는 상태가 이 시간 이상 지속되면 다른 대상을 찾음
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Approach")
	float NoSlotRetargetDelay = 1.5f;

	// 접근 정체 감시: 사거리 밖인데 이 시간 동안 거의 움직이지 않으면 자리를 바꿈
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Approach")
	float ApproachStallTime = 0.8f;

	// 정체로 포기한 자리를 다시 고르지 않는 시간
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Approach")
	float BlockedSlotMemory = 1.5f;

	// 정체가 이 횟수만큼 반복되면 대상을 바꿈
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Approach")
	int32 MaxApproachStallCount = 2;

	float ApproachMovingTime = 0.f;
	int32 ApproachStallCount = 0;
	float BlockedSlotYaw = 0.f;
	float BlockedSlotTime = -1000.f;
	bool bForceRepath = false;

	bool HandleApproachStall(AActor* Target);

	/************************************/
	/***************Move*****************/
	/************************************/
	void MoveAlongLane();
	void MoveTowards(const FVector& Goal, float AcceptanceRadius, const AActor* IgnoreActor);
	void UpdateMoveGoal(const FVector& Goal, float AcceptanceRadius);
	void FaceMoveDirection(const FVector& MoveGoal);

	FVector LastMoveGoal = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "Minion AI")
	float LaneMoveAcceptanceRadius = 50.f;

	// 이동 목표가 이 거리 이상 바뀌었을 때만 경로 다시 요청
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI")
	float MoveGoalUpdateThreshold = 100.f;

	// 이동 목표에 이 거리 안으로 가까워지면 바라보는 방향 고정 (도착 직전 제자리 회전 방지)
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Approach")
	float FacingMinDistance = 80.f;

	/************************************/
	/***************Detour***************/
	/************************************/
	FVector ResolveMoveGoal(const FVector& Goal, const AActor* IgnoreActor);
	bool FindDetourPoint(const FVector& Goal, const AActor* IgnoreActor, bool bIncludeMovingUnits, int32 AvoidSide, FVector& OutPoint, int32& OutSide) const;
	bool IsDetourPointReached() const;
	bool ConsumeDetourArrival();
	bool CheckStuck();
	void ClearDetour() { bHasDetourPoint = false; }

	bool bHasDetourPoint = false;
	FVector DetourPoint = FVector::ZeroVector;
	float DetourStartTime = 0.f;
	int32 DetourSide = 0;				// +1 오른쪽, -1 왼쪽
	int32 FailedDetourSide = 0;			// 직전에 실패한 우회 방향 (다음엔 반대쪽)
	bool bDetourFailedPending = false;	// 우회 실패 → 다음 판단에서 대상 재선택

	float LastStuckCheckTime = 0.f;
	FVector LastStuckCheckLocation = FVector::ZeroVector;

	// 목표까지 이 거리 안에서는 우회 계산을 하지 않음 (마지막 접근은 Crowd 회피에 맡김)
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Detour")
	float DetourMinGoalDistance = 250.f;

	// 진행 방향 앞쪽 이 거리 안에서 멈춰 있는 유닛이 길을 막으면 미리 옆으로 돌아감
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Detour")
	float BlockCheckDistance = 400.f;

	// 이 속도(cm/s) 이하인 유닛을 멈춰 있는 장애물로 간주
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Detour")
	float StationarySpeedThreshold = 50.f;

	// 장애물 옆을 지날 때 추가로 띄우는 간격
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Detour")
	float DetourClearance = 40.f;

	// 장애물을 완전히 지나친 뒤 이만큼 더 앞까지 가서 복귀
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Detour")
	float DetourPassMargin = 80.f;

	// 우회 지점에 이 거리 안으로 들어가면 도착
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Detour")
	float DetourReachRadius = 60.f;

	// 우회 지점에 이 시간 안에 도착하지 못하면 실패
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Detour")
	float DetourTimeout = 1.5f;

	// 이동 중 이 간격 동안 StuckMinMove 미만으로 움직였으면 갇힌 것으로 판단
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Detour")
	float StuckCheckInterval = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Detour")
	float StuckMinMove = 30.f;

	/************************************/
	/***************Crowd****************/
	/************************************/
	class UCrowdFollowingComponent* GetCrowdComponent() const;
	void ConfigureCrowdAgent();
	void SetCrowdAgentEnabled(bool bEnabled);

	// 회피 대상으로 고려하는 주변 반경
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Crowd")
	float CrowdCollisionQueryRange = 350.f;

	/************************************/
	/**************Helpers***************/
	/************************************/
	class AMinion* GetMinion() const;
	float GetNow() const;

	// Center 반경 안의 폰을 중복 없이 수집 (내 폰 제외)
	void GatherNearbyPawns(const FVector& Center, float Radius, TArray<AActor*>& OutActors) const;

	// 자리/우회 계산에서 막는 유닛으로 볼지 (구조물·사망 제외, 움직이는 유닛은 bIncludeMovingUnits일 때만)
	bool IsBlockingUnit(const AActor* Actor, bool bIncludeMovingUnits) const;

	static float GetCollisionRadius(const AActor* Actor);
	static bool IsStructure(const AActor* Actor);

	// 리슨 서버 or Standalone에서 확인 가능한 디버깅
	UPROPERTY(EditDefaultsOnly, Category = "Minion AI|Debug")
	bool bDrawDebug = false;
};