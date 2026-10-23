// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/CGameMode.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemInterface.h"
#include "AIController.h"
#include "AI/CAIController.h"
#include "AI/Turret/Turret.h"
#include "AI/Turret/Nexus.h"
#include "AI/MinionBarrack.h"
#include "BrainComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Framework/CAssetManager.h"
#include "Framework/CGameState.h"
#include "Framework/CGameInstance.h"
#include "Framework/CLoadingScreenSubsystem.h"
#include "Framework/StormCore.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/Controller.h"
#include "GAS/CAbilitySystemStatics.h"
#include "Network/CGameSession.h"
#include "Player/CPlayerController.h"
#include "Player/CPlayerState.h"
#include "ProfilingDebugging/CountersTrace.h"
#include "ProfilingDebugging/MiscTrace.h"
#include "AI/Minion.h"
#include "Framework/CoreBench.h"

TRACE_DECLARE_INT_COUNTER(Core_ActiveMinions, TEXT("Core/ActiveMinions"));

ACGameMode::ACGameMode()
{
	GameSessionClass = ACGameSession::StaticClass();
}

APlayerController* ACGameMode::SpawnPlayerController(ENetRole InRemoteRole, const FString& Options)
{
	APlayerController* NewPlayerController = Super::SpawnPlayerController(InRemoteRole, Options);

	IGenericTeamAgentInterface* NewPlayerTeamInterface = Cast<IGenericTeamAgentInterface>(NewPlayerController);
	FGenericTeamId TeamId = GetTeamIDForPlayer(NewPlayerController);

	if (NewPlayerTeamInterface)
	{
		NewPlayerTeamInterface->SetGenericTeamId(TeamId);
	}

	NewPlayerController->StartSpot = FindNextStartSpotForTeam(TeamId);

	return NewPlayerController;
}

void ACGameMode::StartPlay()
{
	Super::StartPlay();

	if ( bUseStormCoreWinCondition )
	{
		if ( AStormCore* StormCore = GetStormCore() )
		{
			StormCore->OnGoalReachedDelegate.AddUObject(this, &ACGameMode::MatchFinished);
		}
	}

	BindNexusEvents();

	BeginWaitingForPlayers();
}

void ACGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);

	ACGameState* CGameState = GetWorld()->GetGameState<ACGameState>();
	if ( !CGameState || CGameState->GetMatchPhase() != EMatchPhase::WaitingForPlayers )
		return;

	if ( Cast<APlayerController>(Exiting) && ExpectedPlayerCount > 0 )
	{
		--ExpectedPlayerCount;
	}

	TryBeginCountdown();
}

UClass* ACGameMode::GetDefaultPawnClassForController_Implementation(AController* Controller)
{
	ACPlayerState* CPlayerState = Controller->GetPlayerState<ACPlayerState>();
	if (CPlayerState && CPlayerState->GetSelectedPawnClass())
	{
		return CPlayerState->GetSelectedPawnClass();
	}

	return BackupPawn;
}

APawn* ACGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	IGenericTeamAgentInterface* NewPlayerTeamInterface = Cast<IGenericTeamAgentInterface>(NewPlayer);
	FGenericTeamId TeamId = GetTeamIDForPlayer(NewPlayer);

	if (NewPlayerTeamInterface)
	{
		NewPlayerTeamInterface->SetGenericTeamId(TeamId);
	}

	StartSpot = FindNextStartSpotForTeam(TeamId);
	NewPlayer->StartSpot = StartSpot;

	return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
}

void ACGameMode::HandleClientReady(ACPlayerController* ReadyController)
{
	UE_LOG(LogTemp, Log, TEXT("[Match] 클라이언트 준비 완료: %s (%d명 준비)"), *GetNameSafe(ReadyController), GetReadyPlayerCount());
	TryBeginCountdown();
}

FGenericTeamId ACGameMode::GetTeamIDForPlayer(const AController* InController) const
{
	ACPlayerState* CPlayerState = InController->GetPlayerState<ACPlayerState>();
	if (CPlayerState && CPlayerState->GetSelectedPawnClass())
	{
		return CPlayerState->GetTeamIdBasedOnSlot();
	}

	static int PlayerCount = 0;
	++PlayerCount;

	return FGenericTeamId(PlayerCount % 2);
}

AActor* ACGameMode::FindNextStartSpotForTeam(const FGenericTeamId& TeamID) const
{
	const FName* StartSpotTag = TeamStartSpotTagMap.Find(TeamID);

	if (!StartSpotTag)
		return nullptr;

	UWorld* World = GetWorld();

	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		if (It->PlayerStartTag == *StartSpotTag)
		{
			It->PlayerStartTag = FName("Taken");

			return *It;
		}
	}

	return nullptr;
}

AStormCore* ACGameMode::GetStormCore() const
{
	UWorld* World = GetWorld();

	if (World)
	{	
		for (TActorIterator<AStormCore> It(World); It; ++It)
		{
			return *It;
		}
	}

	return nullptr;
}

void ACGameMode::MatchFinished(AActor* ViewTarget, int WinningTeam)
{
	if ( bMatchFinished )
		return;

	bMatchFinished = true;

	GetWorldTimerManager().ClearTimer(StartCountdownTimerHandle);
	GetWorldTimerManager().ClearTimer(LoadingTimeoutTimerHandle);

	if ( ACGameState* CGameState = GetGameState<ACGameState>() )
	{
		CGameState->SetMatchPhase(EMatchPhase::Finished);
	}

	FreezeGameplay();

	UWorld* World = GetWorld();
	if ( World )
	{
		for ( TActorIterator<ACPlayerController> It(World); It; ++It )
		{
			It->MatchFinished(ViewTarget, WinningTeam);
		}
	}
}

void ACGameMode::BindNexusEvents()
{
	UWorld* World = GetWorld();
	if ( !World )
		return;

	for ( TActorIterator<ANexus> It(World); It; ++It )
	{
		It->OnNexusDestroyed.AddUObject(this, &ACGameMode::HandleNexusDestroyed);
	}
}

void ACGameMode::HandleNexusDestroyed(ANexus* DestroyedNexus)
{
	if ( bMatchFinished || !DestroyedNexus )
		return;

	bMatchFinished = true;

	const int32 LosingTeam = DestroyedNexus->GetGenericTeamId().GetId();
	const int32 WinningTeam = (LosingTeam == 0) ? 1 : 0;

	GetWorldTimerManager().ClearTimer(StartCountdownTimerHandle);
	GetWorldTimerManager().ClearTimer(LoadingTimeoutTimerHandle);

	if ( ACGameState* CGameState = GetGameState<ACGameState>() )
	{
		CGameState->SetMatchPhase(EMatchPhase::Finished);
	}

	FreezeGameplay();
	
	const float BlendDuration = DestroyedNexus->GetCameraBlendDuration();
	const float ResultDelay = DestroyedNexus->GetResultWidgetDelay();

	for ( TActorIterator<ACPlayerController> It(GetWorld()); It; ++It )
	{
		It->MatchFinished(DestroyedNexus, WinningTeam, BlendDuration, ResultDelay);
	}

	if ( BlendDuration > 0.f )
	{
		GetWorldTimerManager().SetTimer(NexusExplosionTimerHandle, FTimerDelegate::CreateUObject(DestroyedNexus, &ANexus::StartExplosion), BlendDuration, false);
	}
	else
	{
		DestroyedNexus->StartExplosion();
	}
}

void ACGameMode::FreezeGameplay()
{
	UWorld* World = GetWorld();
	if ( !World )
		return;

	for ( TActorIterator<AMinionBarrack> It(World); It; ++It )
	{
		It->StopSpawning();
	}

	for ( TActorIterator<ATurret> It(World); It; ++It )
	{
		It->StopCombat();
	}

	for ( TActorIterator<AActor> It(World); It; ++It )
	{
		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(*It);
		UAbilitySystemComponent* ASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;

		if ( !ASC )
			continue;

		ASC->CancelAllAbilities();
		ASC->AddLooseGameplayTag(UCAbilitySystemStatics::GetInvincibleStatusTag());
	}

	for ( TActorIterator<ACharacter> It(World); It; ++It )
	{
		ACharacter* Character = *It;

		if ( UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement() )
		{
			MoveComp->StopMovementImmediately();
		}

		if ( ACAIController* CAIC = Cast<ACAIController>(Character->GetController()) )
		{
			CAIC->StopAI();
		}
		else if ( AAIController* AIC = Cast<AAIController>(Character->GetController()) )
		{
			AIC->StopMovement();

			if ( AIC->BrainComponent )
			{
				AIC->BrainComponent->StopLogic(TEXT("Match Finished"));
			}
		}
	}
}

void ACGameMode::BeginWaitingForPlayers()
{
	WaitStartTime = GetWorld()->GetTimeSeconds();

	if ( UCGameInstance* CGameInstance = GetWorld()->GetGameInstance<UCGameInstance>() )
	{
		ExpectedPlayerCount = CGameInstance->ConsumeExpectedMatchPlayerCount();
	}

	UE_LOG(LogTemp, Log, TEXT("[Match] 플레이어 대기 시작 - 예상 인원 %d명 (0이면 로비 없이 실행)"), ExpectedPlayerCount);

	if ( UCLoadingScreenSubsystem::ShouldSkipMatchLoading(this) )
	{
		UE_LOG(LogTemp, Log, TEXT("[Match] PIE - 로딩 대기와 카운트다운을 건너뜁니다."));
		UCAssetManager::Get().PreloadMatchResources(false, FSimpleDelegate::CreateUObject(this, &ACGameMode::StartMatch));
		return;
	}

	GetWorldTimerManager().SetTimer(LoadingTimeoutTimerHandle, this, &ACGameMode::HandleLoadingTimeout, LoadingTimeout, false);

	if ( ExpectedPlayerCount <= 0 )
	{
		GetWorldTimerManager().SetTimer(NoLobbyWaitTimerHandle, this, &ACGameMode::TryBeginCountdown, MinWaitWithoutLobby, false);
	}

	UCAssetManager::Get().PreloadMatchResources(false, FSimpleDelegate::CreateUObject(this, &ACGameMode::HandleServerPreloadFinished));
}

void ACGameMode::HandleServerPreloadFinished()
{
	bServerResourcesReady = true;
	TryBeginCountdown();
}

void ACGameMode::HandleLoadingTimeout()
{
	bLoadingTimedOut = true;
	UE_LOG(LogTemp, Warning, TEXT("[Match] 로딩 타임아웃 - 준비된 %d명으로 시작합니다."), GetReadyPlayerCount());
	TryBeginCountdown();
}

void ACGameMode::TryBeginCountdown()
{
	ACGameState* CGameState = GetGameState<ACGameState>();
	if ( !CGameState || CGameState->GetMatchPhase() != EMatchPhase::WaitingForPlayers || !bServerResourcesReady )
		return;

	const int32 ReadyCount = GetReadyPlayerCount();

	if ( bLoadingTimedOut )
	{
		if ( ReadyCount > 0 )
		{
			BeginCountdown();
		}

		return;
	}

	int32 RequiredCount = ExpectedPlayerCount;

	if ( RequiredCount <= 0 )
	{
		if ( GetWorld()->GetTimeSeconds() - WaitStartTime < MinWaitWithoutLobby )
			return;

		RequiredCount = GetNumPlayers();
	}

	if ( RequiredCount > 0 && ReadyCount >= RequiredCount )
	{
		BeginCountdown();
	}
}

void ACGameMode::BeginCountdown()
{
	ACGameState* CGameState = GetGameState<ACGameState>();
	if ( !CGameState )
		return;

	GetWorldTimerManager().ClearTimer(LoadingTimeoutTimerHandle);
	GetWorldTimerManager().ClearTimer(NoLobbyWaitTimerHandle);

	const float TotalDuration = MatchStartAnnounceDuration + StartCountdownDuration;

	if ( TotalDuration <= 0.f )
	{
		StartMatch();
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Match] 카운트다운 시작 (안내 %.1f초 + 카운트 %.1f초)"), MatchStartAnnounceDuration, StartCountdownDuration);

	CGameState->SetMatchPhase(EMatchPhase::Countdown, StartCountdownDuration, MatchStartAnnounceDuration);
	GetWorldTimerManager().SetTimer(StartCountdownTimerHandle, this, &ACGameMode::StartMatch, TotalDuration, false);
}

void ACGameMode::StartMatch()
{
	ACGameState* CGameState = GetGameState<ACGameState>();
	if ( !CGameState || bMatchFinished || CGameState->GetMatchPhase() == EMatchPhase::InProgress )
		return;

	UE_LOG(LogTemp, Log, TEXT("[Match] 게임 시작 !"));

	CGameState->SetMatchPhase(EMatchPhase::InProgress);
	CGameState->SetFirstWavePlannedTime(FirstMinionWaveDelay);

	UWorld* World = GetWorld();

	for ( TActorIterator<AMinionBarrack> It(World); It; ++It )
	{
		It->StartSpawning(FirstMinionWaveDelay);
	}

	for ( TActorIterator<ATurret> It(World); It; ++It )
	{
		It->StartCombat();
	}

	const FGameplayTag NotStartedTag = UCAbilitySystemStatics::GetMatchNotStartedStatusTag();

	for ( FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It )
	{
		APlayerController* PC = It->Get();
		UAbilitySystemComponent* ASC = PC ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PC->GetPawn()) : nullptr;

		if ( ASC && ASC->GetTagCount(NotStartedTag) > 0 )
		{
			ASC->RemoveLooseGameplayTag(NotStartedTag);
			ASC->RemoveReplicatedLooseGameplayTag(NotStartedTag);
		}
	}

	TRACE_BOOKMARK(TEXT("Match Start"));
	UE_LOG(LogTemp, Warning, TEXT("[CoreBench] OptOff MinionAnim=%d DeathAnim=%d PopupFilter=%d Projectile=%d"),
		CoreBench::IsOptOff(TEXT("NoOptMinionAnim")),
		CoreBench::IsOptOff(TEXT("NoOptDeathAnim")),
		CoreBench::IsOptOff(TEXT("NoOptPopupFilter")),
		CoreBench::IsOptOff(TEXT("NoOptProjectile")));

	// 측정용: 0.5초마다 살아 있는 미니언 수 기록
	GetWorldTimerManager().SetTimer(MinionCountTraceTimerHandle, FTimerDelegate::CreateWeakLambda(this, [ this ] ()
		{
			int32 Count = 0;
			for ( TActorIterator<AMinion> It(GetWorld()); It; ++It )
			{
				if ( It->IsActive() )
					++Count;
			}
			TRACE_COUNTER_SET(Core_ActiveMinions, Count);
		}), 0.5f, true);
}

int32 ACGameMode::GetReadyPlayerCount() const
{
	int32 Count = 0;

	if ( const AGameStateBase* GS = GetGameState<AGameStateBase>() )
	{
		for ( APlayerState* PS : GS->PlayerArray )
		{
			const ACPlayerState* CPS = Cast<ACPlayerState>(PS);
			if ( CPS && !CPS->IsInactive() && CPS->IsLoadingComplete() )
			{
				++Count;
			}
		}
	}

	return Count;
}

