// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/CPlayerController.h"
#include "AbilitySystemComponent.h"
#include "AI/Turret/Turret.h"
#include "Character/CCharacter.h"
#include "EngineUtils.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Framework/CAssetManager.h"
#include "Framework/CGameMode.h"
#include "Framework/CGameState.h"
#include "Framework/CLoadingScreenSubsystem.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "Player/ChatComponent.h"
#include "Player/CPlayerCharacter.h"
#include "Player/CPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Widgets/GameplayWidget.h"

ACPlayerController::ACPlayerController()
{
	ChatComponent = CreateDefaultSubobject<UChatComponent>("Chat Component");
}

void ACPlayerController::OnPossess(APawn* NewPawn)
{
	Super::OnPossess(NewPawn);
	
	CPlayerCharacter = Cast<ACPlayerCharacter>(NewPawn);
	if (CPlayerCharacter)
	{
		CPlayerCharacter->ServerSideInit();
		CPlayerCharacter->SetGenericTeamId(TeamID);

		if ( !ACGameState::IsMatchInProgress(this) )
		{
			if ( UAbilitySystemComponent* ASC = CPlayerCharacter->GetAbilitySystemComponent() )
			{
				ASC->AddLooseGameplayTag(UCAbilitySystemStatics::GetMatchNotStartedStatusTag());
				ASC->AddReplicatedLooseGameplayTag(UCAbilitySystemStatics::GetMatchNotStartedStatusTag());
			}
		}
	}
}

void ACPlayerController::AcknowledgePossession(APawn* NewPawn)
{
	Super::AcknowledgePossession(NewPawn);

	CPlayerCharacter = Cast<ACPlayerCharacter>(NewPawn);
	if (CPlayerCharacter)
	{
		CPlayerCharacter->ClientSideInit();
		SpawnGameplayWidget();

		bClientPawnReady = true;
		TryReportClientReady();

		ACCharacter::RefreshAllTeamVisuals(this);
	}
}

void ACPlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	TeamID = NewTeamID;
}

FGenericTeamId ACPlayerController::GetGenericTeamId() const
{
	return TeamID;
}

void ACPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACPlayerController, TeamID);
}

void ACPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

	if (InputSubsystem)
	{
		InputSubsystem->RemoveMappingContext(UIInputMapping);
		InputSubsystem->AddMappingContext(UIInputMapping, 1);
	}

	UEnhancedInputComponent* EnhancedInputComp = Cast<UEnhancedInputComponent>(InputComponent);

	if (EnhancedInputComp)
	{
		EnhancedInputComp->BindAction(ShopToggleInputAction, ETriggerEvent::Triggered, this, &ACPlayerController::ToggleShop);
		EnhancedInputComp->BindAction(ToggleGameplayMenuAction, ETriggerEvent::Triggered, this, &ACPlayerController::ToggleGameplayMenu);
		EnhancedInputComp->BindAction(FocusChatInputAction, ETriggerEvent::Triggered, this, &ACPlayerController::FocusChatInput);

		EnhancedInputComp->BindAction(ScoreboardInputAction, ETriggerEvent::Started, this, &ACPlayerController::ShowScoreboard);
		EnhancedInputComp->BindAction(ScoreboardInputAction, ETriggerEvent::Completed, this, &ACPlayerController::HideScoreboard);
	}
}

void ACPlayerController::MatchFinished(AActor* ViewTarget, int WinningTeam, float ViewBlendDuration, float ResultWidgetDelay)
{
	if ( !HasAuthority() )
		return;

	if( CPlayerCharacter )
	{
		CPlayerCharacter->SetCameraInputBlock(true);
	}

	Client_MatchFinished(ViewTarget, WinningTeam, ViewBlendDuration, ResultWidgetDelay);
}

void ACPlayerController::GrantInBaseTag()
{
	CPlayerCharacter->GrantInBaseTag();
}

void ACPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if ( IsLocalPlayerController() )
	{
		StartClientLoading();
	}
}

void ACPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if ( IsLocalPlayerController() && GetLocalPlayer() )
	{
		StartClientLoading();
	}
}

void ACPlayerController::Client_MatchFinished_Implementation(AActor* ViewTarget, int WinningTeam, float ViewBlendDuration, float ResultWidgetDelay)
{
	const float BlendTime = ViewBlendDuration >= 0.f ? ViewBlendDuration : MatchFinishViewBlendTimeDuration;

	if ( GameplayWidget )
	{
		GameplayWidget->EnterMatchFinishedState();
	}

	if ( APawn* ControlledPawn = GetPawn() )
	{
		if ( UPawnMovementComponent* MovementComp = ControlledPawn->GetMovementComponent() )
		{
			MovementComp->StopMovementImmediately();
		}
	}

	if ( UWorld* World = GetWorld() )
	{
		for ( TActorIterator<ACCharacter> It(World); It; ++It )
		{
			It->HideOverheadStatsGauge();
		}

		for ( TActorIterator<ATurret> It(World); It; ++It )
		{
			It->HideOverheadStatsGauge();
		}
	}

	SetViewTargetWithBlend(ViewTarget, BlendTime, VTBlend_EaseInOut, MatchFinishViewBlendExp);

	FString WinLoseMsg = TEXT("승리");
	if (GetGenericTeamId().GetId() != WinningTeam)
	{
		WinLoseMsg = TEXT("패배");
	}

	if ( GameplayWidget )
	{
		GameplayWidget->SetGameplayMenuTitle(WinLoseMsg);
	}

	const float ShowDelay = BlendTime + FMath::Max(0.f, ResultWidgetDelay);
	if ( ShowDelay > 0.f )
	{
		GetWorldTimerManager().SetTimer(ShowWinLoseStateTimerHandle, this, &ACPlayerController::ShowWinLoseState, ShowDelay, false);
	}
	else
	{
		ShowWinLoseState();
	}
}

void ACPlayerController::SpawnGameplayWidget()
{
	if (!IsLocalPlayerController())
		return;

	GameplayWidget = CreateWidget<UGameplayWidget>(this, GameplayWidgetclass);
	if (GameplayWidget)
	{
		GameplayWidget->AddToViewport();
		GameplayWidget->ConfigureAbilities(CPlayerCharacter->GetAbilities());
	}
}

void ACPlayerController::ToggleShop()
{
	if ( IsLoadingScreenVisible() || !ACGameState::IsMatchInProgress(this) )
		return;

	if (GameplayWidget)
	{
		GameplayWidget->ToggleShop();
	}
}

void ACPlayerController::ToggleGameplayMenu()
{
	if (GameplayWidget)
	{
		GameplayWidget->ToggleGameplayMenu();
	}
}

void ACPlayerController::FocusChatInput()
{
	if ( GameplayWidget )
	{
		GameplayWidget->FocusChatInput();
	}
}

void ACPlayerController::ShowWinLoseState()
{
	if (GameplayWidget)
	{
		GameplayWidget->SetVisibility(ESlateVisibility::Visible);
		GameplayWidget->ShowGameplayMenu();
	}
}

void ACPlayerController::ShowScoreboard()
{
	/*if ( IsLoadingScreenVisible() || !ACGameState::IsMatchInProgress(this) )
		return;*/

	if ( GameplayWidget )
	{
		GameplayWidget->ShowScoreboard();
	}
}

void ACPlayerController::HideScoreboard()
{
	if ( GameplayWidget )
	{
		GameplayWidget->HideScoreboard();
	}
}

void ACPlayerController::StartClientLoading()
{
	if ( bClientLoadingStarted )
		return;

	bClientLoadingStarted = true;

	if ( UCLoadingScreenSubsystem* LoadingSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UCLoadingScreenSubsystem>() : nullptr )
	{
		LoadingSubsystem->ShowLoadingScreen();
	}

	GetWorldTimerManager().SetTimer(LoadingProgressTimerHandle, this, &ACPlayerController::UpdateLoadingProgress, 0.25f, true);

	UCAssetManager::Get().PreloadMatchResources(true, FSimpleDelegate::CreateUObject(this, &ACPlayerController::HandleClientPreloadFinished));
}

void ACPlayerController::HandleClientPreloadFinished()
{
	bClientAssetsLoaded = true;
	TryReportClientReady();
}

void ACPlayerController::UpdateLoadingProgress()
{
	const float AssetProgress = UCAssetManager::Get().GetMatchPreloadProgress(true);
	int32 Progress = 70 + FMath::FloorToInt(AssetProgress * 25.f) + (bClientPawnReady ? 5 : 0);

	if ( bClientReadyReported )
	{
		Progress = 100;
	}

	LocalLoadingProgress = ( uint8 ) FMath::Clamp(Progress, 0, 100);

	if ( LocalLoadingProgress != LastSentLoadingProgress )
	{
		LastSentLoadingProgress = LocalLoadingProgress;
		Server_ReportLoadingProgress(LocalLoadingProgress);
	}

	if ( bClientReadyReported )
	{
		GetWorldTimerManager().ClearTimer(LoadingProgressTimerHandle);
	}
}

void ACPlayerController::TryReportClientReady()
{
	if ( bClientReadyReported || !bClientAssetsLoaded || !bClientPawnReady )
		return;

	bClientReadyReported = true;
	LocalLoadingProgress = 100;

	Server_ReportClientReady();
}

bool ACPlayerController::IsLoadingScreenVisible() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UCLoadingScreenSubsystem* LoadingSubsystem = GameInstance ? GameInstance->GetSubsystem<UCLoadingScreenSubsystem>() : nullptr;

	return LoadingSubsystem && LoadingSubsystem->IsLoadingScreenVisible();
}

void ACPlayerController::Server_ReportClientReady_Implementation()
{
	if ( ACPlayerState* CPS = GetPlayerState<ACPlayerState>() )
	{
		CPS->SetLoadingComplete();
	}

	if ( ACGameMode* CGameMode = GetWorld()->GetAuthGameMode<ACGameMode>() )
	{
		CGameMode->HandleClientReady(this);
	}
}

void ACPlayerController::Server_ReportLoadingProgress_Implementation(uint8 Progress)
{
	if ( ACPlayerState* CPS = GetPlayerState<ACPlayerState>() )
	{
		CPS->SetLoadingProgress(FMath::Min<uint8>(Progress, 99));
	}
}
