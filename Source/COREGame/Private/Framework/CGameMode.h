// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GenericTeamAgentInterface.h"

#include "CGameMode.generated.h"

/**
 * 
 */
UCLASS()
class ACGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACGameMode();
	virtual APlayerController* SpawnPlayerController(ENetRole InRemoteRole, const FString& Options) override;
	virtual void StartPlay() override;
	virtual void Logout(AController* Exiting) override;
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* Controller) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

	void HandleClientReady(class ACPlayerController* ReadyController);

private:
	FGenericTeamId GetTeamIDForPlayer(const AController* InController) const;

	AActor* FindNextStartSpotForTeam(const FGenericTeamId& TeamID) const;

	UPROPERTY(EditDefaultsOnly, Category = "Team")
	TSubclassOf<APawn> BackupPawn;

	UPROPERTY(EditDefaultsOnly, Category = "Team")
	TMap<FGenericTeamId, FName> TeamStartSpotTagMap;

	class AStormCore* GetStormCore() const;

	void MatchFinished(AActor* ViewTarget, int WinningTeam);

	UPROPERTY(EditDefaultsOnly, Category = "Match")
	bool bUseStormCoreWinCondition = false;

	void BindNexusEvents();
	void HandleNexusDestroyed(class ANexus* DestroyedNexus);

	// 경기 종료 시 모든 전투/이동/생성 정지
	void FreezeGameplay();

	bool bMatchFinished = false;
	FTimerHandle NexusExplosionTimerHandle;

	/***************************************/
	/**************Match Start**************/
	/***************************************/
	
	//이 시간 안에 모두 준비가 안되면 준비된 인원만으로 시작
	UPROPERTY(EditDefaultsOnly, Category = "Match|Start")
	float LoadingTimeout = 60.f;

	//로딩 화면이 닫힌 뒤 "게임이 N초 뒤 시작됩니다" 표시 시간
	UPROPERTY(EditDefaultsOnly, Category = "Match|Start")
	float MatchStartAnnounceDuration = 3.f;

	//안내 후 숫자로 세는 카운트다운
	UPROPERTY(EditDefaultsOnly, Category = "Match|Start")
	float StartCountdownDuration = 5.f;

	//첫번째 미니언 웨이브 나오는 시간
	UPROPERTY(EditDefaultsOnly, Category = "Match|Start")
	float FirstMinionWaveDelay = 30.f;

	//로비 없이 게임 맵을 바로 시작했을 때 최소 접속 대기 시간
	UPROPERTY(EditDefaultsOnly, Category = "Match|Start")
	float MinWaitWithoutLobby = 3.f;

	void BeginWaitingForPlayers();
	void HandleServerPreloadFinished();
	void HandleLoadingTimeout();
	void TryBeginCountdown();
	void BeginCountdown();
	void StartMatch();

	int32 GetReadyPlayerCount() const;

	int32 ExpectedPlayerCount = 0;
	bool bServerResourcesReady = false;
	bool bLoadingTimedOut = false;
	float WaitStartTime = 0.f;

	FTimerHandle LoadingTimeoutTimerHandle;
	FTimerHandle NoLobbyWaitTimerHandle;
	FTimerHandle StartCountdownTimerHandle;

	FTimerHandle MinionCountTraceTimerHandle;
};
