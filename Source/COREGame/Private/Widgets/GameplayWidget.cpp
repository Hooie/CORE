// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/GameplayWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Character/CMurdockCharacter.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/WidgetSwitcher.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/CGameInstance.h"
#include "Framework/CGameState.h"
#include "Kismet/GameplayStatics.h"
#include "GAS/CAbilitySystemComponent.h"
#include "GAS/CAttributeSet.h"
#include "GAS/CHeroAttributeSet.h"
#include "Player/ChatTypes.h"
#include "Player/CPlayerState.h"
#include "Widgets/AmmoWidget.h"
#include "Widgets/AbilityListView.h"
#include "Widgets/ChatWidget.h"
#include "Widgets/GameplayMenu.h"
#include "Widgets/GoldPopupWidget.h"
#include "Widgets/HoldProgressWidget.h"
#include "Widgets/ShopWidget.h"
#include "Widgets/UltimateAbilityGauge.h"
#include "Widgets/ValueGauge.h"
#include "Widgets/ScoreboardWidget.h"
#include "Kismet/KismetSystemLibrary.h"

class FGameChatClickAwayProcessor : public IInputProcessor
{
public:
	FGameChatClickAwayProcessor(TWeakObjectPtr<UGameplayWidget> InOwner) : Owner(InOwner) {}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}

	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		if ( !Owner.IsValid() )
			return false;

		return Owner->HandleGameViewportClicked(MouseEvent);
	}

private:
	TWeakObjectPtr<UGameplayWidget> Owner;
};

void UGameplayWidget::NativeConstruct()
{
	Super::NativeConstruct();

	OwnerAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwningPlayerPawn());

	/*const UCAbilitySystemComponent* CAbilitySystemComponent = Cast<UCAbilitySystemComponent>(OwnerAbilitySystemComponent);

	if (CAbilitySystemComponent)
	{
		ConfigureAbilities(CAbilitySystemComponent->GetAbilities());
	}*/

	if (OwnerAbilitySystemComponent)
	{
		HealthBar->SetAndBoundToGameplayAttribute(OwnerAbilitySystemComponent, UCAttributeSet::GetHealthAttribute(), UCAttributeSet::GetMaxHealthAttribute(), UCHeroAttributeSet::GetHealthRegenAttribute(), UCAttributeSet::GetShieldAttribute());
		ManaBar->SetAndBoundToGameplayAttribute(OwnerAbilitySystemComponent, UCAttributeSet::GetManaAttribute(), UCAttributeSet::GetMaxManaAttribute(), UCHeroAttributeSet::GetManaRegenAttribute());

		bool bGoldFound = false;
		LastKnownGold = OwnerAbilitySystemComponent->GetGameplayAttributeValue(UCHeroAttributeSet::GetGoldAttribute(), bGoldFound);
		if ( !bGoldFound )
		{
			LastKnownGold = 0.f;
		}

		OwnerAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UCHeroAttributeSet::GetGoldAttribute()).AddUObject(this, &UGameplayWidget::GoldValueChanged);
		OwnerAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UCHeroAttributeSet::GetLevelAttribute()).AddUObject(this, &UGameplayWidget::LevelChanged);
	}

	SetShowMouseCursor(false);
	SetFocusToGameOnly();

	QuitConfirmBtn->OnClicked.AddDynamic(this, &UGameplayWidget::QuitConfirmBtnClicked);
	QuitCancelBtn->OnClicked.AddDynamic(this, &UGameplayWidget::QuitCancelBtnClicked);

	if (GameplayMenu)
	{
		GameplayMenu->GetResumeButtonClickedEventDelegate().AddDynamic(this, &UGameplayWidget::ToggleGameplayMenu);
		GameplayMenu->OnQuitMatchBtnClicked.AddUObject(this, &UGameplayWidget::QuitMatchBtnClicked);
	}

	if ( ChatWidget )
	{
		ChatWidget->OnChatInputFinished.AddUObject(this, &UGameplayWidget::OnChatInputFinished);
		ChatWidget->OnChatInputRequested.AddUObject(this, &UGameplayWidget::OnChatInputRequested);
	}

	if ( ScoreboardWidget )
	{
		ScoreboardWidget->SetVisibility(ESlateVisibility::Collapsed);
		GetWorld()->GetTimerManager().SetTimer(ScoreboardRefreshTimerHandle, ScoreboardWidget, &UScoreboardWidget::RefreshScoreboard, 0.5f, true);
	}

	GetWorld()->GetTimerManager().SetTimer(MatchTimerRefreshHandle, this, &UGameplayWidget::RefreshMatchTimer, 1.f, true, 0.f);
	GetWorld()->GetTimerManager().SetTimer(RespawnCheckTimerHandle, this, &UGameplayWidget::RefreshRespawnOverlay, 0.1f, true);

	if ( RespawnOverlay )
	{
		RespawnOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}

	if ( MatchStartCountdownOverlay )
	{
		MatchStartCountdownOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}

	GetWorld()->GetTimerManager().SetTimer(MatchStartCountdownTimerHandle, this, &UGameplayWidget::RefreshMatchStartCountdown, 0.05f, true, 0.f);

	ChatClickAwayProcessor = MakeShared<FGameChatClickAwayProcessor>(TWeakObjectPtr<UGameplayWidget>(this));
	FSlateApplication::Get().RegisterInputPreProcessor(ChatClickAwayProcessor);

	if ( HoldProgressWidget )
		HoldProgressWidget->SetVisibility(ESlateVisibility::Collapsed);

	if ( AmmoWidget )
	{
		AmmoWidget->SetVisibility(Cast<ACMurdockCharacter>(GetOwningPlayerPawn()) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UGameplayWidget::NativeDestruct()
{
	GetWorld()->GetTimerManager().ClearTimer(ScoreboardRefreshTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(MatchTimerRefreshHandle);
	GetWorld()->GetTimerManager().ClearTimer(RespawnCheckTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(MatchStartCountdownTimerHandle);

	if ( ChatClickAwayProcessor.IsValid() )
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(ChatClickAwayProcessor);
		ChatClickAwayProcessor.Reset();
	}

	if ( OwnerAbilitySystemComponent )
	{
		OwnerAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UCHeroAttributeSet::GetLevelAttribute()).RemoveAll(this);
		OwnerAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UCHeroAttributeSet::GetGoldAttribute()).RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UGameplayWidget::ConfigureAbilities(const TMap<ECAbilityInputID, TSubclassOf<UGameplayAbility>>& Abilities)
{
	TMap<ECAbilityInputID, TSubclassOf<UGameplayAbility>> BasicAbilities;
	TSubclassOf<UGameplayAbility> UltimateAbility;

	for ( const TPair<ECAbilityInputID, TSubclassOf<UGameplayAbility>>& Pair : Abilities )
	{
		if ( Pair.Key == ECAbilityInputID::Ultimate )
			UltimateAbility = Pair.Value;
		
		else
			BasicAbilities.Add(Pair.Key, Pair.Value);
	}

	AbilityListView->ConfigureAbilities(BasicAbilities);

	if ( UltimateAbility && UltimateGauge )
	{
		UltimateGauge->NativeOnListItemObjectSet(UltimateAbility.GetDefaultObject());
		UltimateGauge->ConfigureWithWidgetData(AbilityListView->FindWidgetDataForAbility(UltimateAbility));
	}
}

void UGameplayWidget::ToggleShop()
{
	if (bMatchFinished)
		return;

	if (ShopWidget->GetVisibility() == ESlateVisibility::HitTestInvisible)
	{
		ShopWidget->SetVisibility(ESlateVisibility::Visible);
		ShopWidget->SetShopOpen(true);
		PlayShopPopupAnimation(true);
		SetOwningPawnInputEnabled(false);
		SetShowMouseCursor(true);
		SetFocusToGameAndUI();
		ShopWidget->SetFocus();
	}
	else
	{
		ShopWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		ShopWidget->SetShopOpen(false);
		PlayShopPopupAnimation(false);
		SetOwningPawnInputEnabled(true);
		SetShowMouseCursor(false);
		SetFocusToGameOnly();
	}
}

void UGameplayWidget::ToggleGameplayMenu()
{
	if (bMatchFinished)
		return;

	if ( ShopWidget->GetVisibility() == ESlateVisibility::Visible )
	{
		ToggleShop();
		return;
	}

	if ( MainSwitcher->GetActiveWidget() == GameplayMenuRootPanel )
	{
		MainSwitcher->SetActiveWidget(GameplayWidgetRootPanel);
		GameplayMenu->ReturnToMenu();
		SetOwningPawnInputEnabled(true);
		SetShowMouseCursor(false);
		SetFocusToGameOnly();
	}
	else
	{
		ShowGameplayMenu();
	}
}

void UGameplayWidget::ShowGameplayMenu()
{
	MainSwitcher->SetActiveWidget(GameplayMenuRootPanel);
	SetOwningPawnInputEnabled(false);
	SetShowMouseCursor(true);
	SetFocusToGameAndUI();
}

void UGameplayWidget::SetGameplayMenuTitle(const FString& NewTitle)
{
	GameplayMenu->SetTitleText(NewTitle);
}

void UGameplayWidget::FocusChatInput()
{
	if ( !ChatWidget || ChatWidget->IsInputFocused() )
		return;

	FModifierKeysState Modifiers = FSlateApplication::Get().GetModifierKeys();
	EChatChannel Channel = Modifiers.IsShiftDown() ? EChatChannel::All : EChatChannel::Team;

	ChatWidget->FocusInputWithChannel(Channel);
}

bool UGameplayWidget::HandleGameViewportClicked(const FPointerEvent& MouseEvent)
{
	if ( !ChatWidget || !ChatWidget->IsInputFocused() )
		return false;

	if ( ChatWidget->GetChatInputGeometry().IsUnderLocation(MouseEvent.GetScreenSpacePosition()) )
		return false;

	OnChatInputFinished(); 

	return false;
}

void UGameplayWidget::ShowScoreboard()
{
	if ( ScoreboardWidget )
	{
		ScoreboardWidget->SetVisibility(ESlateVisibility::Visible);
	}
}

void UGameplayWidget::HideScoreboard()
{
	if ( ScoreboardWidget )
	{
		ScoreboardWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UGameplayWidget::ShowHoldProgress(float MaxHoldTime, bool bCountDown)
{
	if ( HoldProgressWidget )
		HoldProgressWidget->StartHold(MaxHoldTime, bCountDown);
}

void UGameplayWidget::HideHoldProgress()
{
	if ( HoldProgressWidget )
		HoldProgressWidget->StopHold();
}

void UGameplayWidget::EnterMatchFinishedState()
{
	if (bMatchFinished)
		return;

	SetVisibility(ESlateVisibility::Hidden);

	if (ShopWidget && ShopWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		ToggleShop();
	}

	if (MainSwitcher && MainSwitcher->GetActiveWidget() != GameplayWidgetRootPanel) 
	{
		MainSwitcher->SetActiveWidget(GameplayWidgetRootPanel);
		GameplayMenu->ReturnToMenu();
	}

	bMatchFinished = true;
	SetOwningPawnInputEnabled(false);

	if (!ChatWidget || !ChatWidget->IsInputFocused())
	{
		SetShowMouseCursor(false);
		SetFocusToGameOnly();
	}

	GetWorld()->GetTimerManager().ClearTimer(MatchTimerRefreshHandle);

	if (GameplayMenu)
	{
		GameplayMenu->SetResumeButtonVisible(false);
	}
}

void UGameplayWidget::PlayShopPopupAnimation(bool bPlayForward)
{
	if (bPlayForward)
	{
		PlayAnimationForward(ShopPopupAnimation);
	}
	else
	{
		PlayAnimationReverse(ShopPopupAnimation);
	}
}

void UGameplayWidget::SetOwningPawnInputEnabled(bool bPawnInputEnabled)
{
	if (bMatchFinished)
	{
		bPawnInputEnabled = false;
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
		return;

	APawn* Pawn = GetOwningPlayerPawn();
	if (!Pawn)
		return;

	if (bPawnInputEnabled)
	{
		Pawn->EnableInput(PC);
	}
	else
	{
		Pawn->DisableInput(PC);
	}
}

void UGameplayWidget::SetShowMouseCursor(bool bShowMouseCursor)
{
	GetOwningPlayer()->SetShowMouseCursor(bShowMouseCursor);
}

void UGameplayWidget::SetFocusToGameAndUI()
{
	FInputModeGameAndUI GameAndUIInputMode;
	GameAndUIInputMode.SetHideCursorDuringCapture(false);

	GetOwningPlayer()->SetInputMode(GameAndUIInputMode);
}

void UGameplayWidget::SetFocusToGameOnly()
{
	FInputModeGameOnly GameOnlyInputMode;

	GetOwningPlayer()->SetInputMode(GameOnlyInputMode);
}

void UGameplayWidget::QuitConfirmBtnClicked()
{
	UCGameInstance* CGameInstance = GetGameInstance<UCGameInstance>();
	if ( !CGameInstance )
	{
		UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, true);
		return;
	}

	CGameInstance->LeaveCurrentSessionAndReturnToMainMenu();
}

void UGameplayWidget::QuitCancelBtnClicked()
{
	if ( GameplayMenuRootPanel )
	{
		MainSwitcher->SetActiveWidget(GameplayMenuRootPanel);
	}
}

void UGameplayWidget::QuitMatchBtnClicked()
{
	if ( QuitCheckWidgetRoot )
	{
		MainSwitcher->SetActiveWidget(QuitCheckWidgetRoot);
	}
}

void UGameplayWidget::OnChatInputFinished()
{
	if ( IsNonGameplayUIOpen() )
		return; 

	SetOwningPawnInputEnabled(true);
	SetShowMouseCursor(false);
	SetFocusToGameOnly();
}

void UGameplayWidget::OnChatInputRequested()
{
	SetOwningPawnInputEnabled(false);
	SetShowMouseCursor(true);
	SetFocusToGameAndUI();
}

bool UGameplayWidget::IsNonGameplayUIOpen() const
{
	return MainSwitcher && MainSwitcher->GetActiveWidget() != GameplayWidgetRootPanel;
}

void UGameplayWidget::RefreshMatchTimer()
{
	if ( !MatchTimerText )
		return;

	ACGameState* CGameState = GetWorld() ? GetWorld()->GetGameState<ACGameState>() : nullptr;
	if ( !CGameState )
		return;

	int32 TotalSeconds = FMath::Max(0, FMath::FloorToInt(CGameState->GetMatchElapsedSeconds()));
	int32 Minutes = TotalSeconds / 60;
	int32 Seconds = TotalSeconds % 60;

	MatchTimerText->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds)));
}

void UGameplayWidget::GoldValueChanged(const FOnAttributeChangeData& Data)
{
	float Delta = Data.NewValue - LastKnownGold;
	LastKnownGold = Data.NewValue;

	if ( Delta == 0.f || !GoldPopupWidgetClass || !GoldPopupAnchor )
		return;

	if ( ActiveGoldPopup )
	{
		ActiveGoldPopup->AddGoldValue(Delta);
		ActiveGoldPopup->PlayPopup();
	}
	else
	{
		UGoldPopupWidget* GoldPopup = CreateWidget<UGoldPopupWidget>(this, GoldPopupWidgetClass);
		if ( !GoldPopup )
			return;

		GoldPopup->SetGoldValue(Delta);
		GoldPopupAnchor->AddChild(GoldPopup);
		
		GoldPopup->OnPopupFinished.AddUObject(this, &UGameplayWidget::OnGoldPopupFinished);

		ActiveGoldPopup = GoldPopup;
		
		GoldPopup->PlayPopup();
	}
}

void UGameplayWidget::OnGoldPopupFinished()
{
	ActiveGoldPopup = nullptr;
}

void UGameplayWidget::RefreshRespawnOverlay()
{
	if ( !RespawnOverlay )
		return;

	ACPlayerState* CPlayerState = Cast<ACPlayerState>(GetOwningPlayerState());
	ACGameState* CGameState = GetWorld()->GetGameState<ACGameState>();

	if ( !CPlayerState || !CGameState )
		return;

	float RespawnTimeStamp = CPlayerState->GetRespawnTimeStamp();

	if ( RespawnTimeStamp < 0.f )
	{
		RespawnOverlay->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	float RemainingTime = RespawnTimeStamp - CGameState->GetServerWorldTimeSeconds();

	if ( RemainingTime <= 0.f )
	{
		RespawnOverlay->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	RespawnOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);

	if ( RespawnTimerText )
	{
		FNumberFormattingOptions FormatOps = FNumberFormattingOptions().SetMaximumFractionalDigits(1);
		RespawnTimerText->SetText(FText::AsNumber(RemainingTime, &FormatOps));
	}
}

void UGameplayWidget::RefreshMatchStartCountdown()
{
	if ( !MatchStartCountdownOverlay || !MatchStartCountdownText )
		return;

	const ACGameState* CGameState = GetWorld() ? GetWorld()->GetGameState<ACGameState>() : nullptr;
	if ( !CGameState )
		return;

	FText Message;
	bool bShow = false;
	bool bAllNoticesDone = false;

	switch ( CGameState->GetMatchPhase() )
	{
	case EMatchPhase::Countdown:
	{
		const float Remaining = CGameState->GetCountdownRemaining();
		const float NumberDuration = CGameState->GetCountdownNumberDuration();

		Message = (Remaining > NumberDuration)
			? FText::Format(MatchStartAnnounceFormat, FText::AsNumber(FMath::RoundToInt(NumberDuration)))	
			: FText::AsNumber(FMath::Max(1, FMath::CeilToInt(Remaining)));									
		bShow = true;
		break;
	}

	case EMatchPhase::InProgress:
	{
		if ( CGameState->GetMatchElapsedSeconds() < MatchStartTextDuration )
		{
			Message = MatchStartText;
			bShow = true;
		}
		else if ( CGameState->HasFirstWaveSpawned() )
		{
			if ( CGameState->GetTimeSinceFirstWave() < MinionNoticeDuration )
			{
				Message = MinionSpawnedText;
				bShow = true;
			}
			else
			{
				bAllNoticesDone = true;
			}
		}
		else
		{
			const float UntilWave = CGameState->GetTimeUntilFirstWave();
			if ( UntilWave >= 0.f && UntilWave <= MinionWarningLeadTime && UntilWave > MinionWarningLeadTime - MinionNoticeDuration )
			{
				Message = FText::Format(MinionWarningFormat, FText::AsNumber(FMath::RoundToInt(MinionWarningLeadTime)));
				bShow = true;
			}
		}
		break;
	}

	case EMatchPhase::Finished:
		bAllNoticesDone = true;
		break;

	default:
		break;
	}

	if ( bShow )
	{
		MatchStartCountdownText->SetText(Message);
		MatchStartCountdownOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		MatchStartCountdownOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}

	if ( bAllNoticesDone )
	{
		GetWorld()->GetTimerManager().ClearTimer(MatchStartCountdownTimerHandle);
	}
}

void UGameplayWidget::LevelChanged(const FOnAttributeChangeData& Data)
{
	if ( Data.OldValue < 1.f )
		return;

	if ( Data.NewValue <= Data.OldValue )
		return;

	if ( LevelUpSound )
	{
		UGameplayStatics::PlaySound2D(this, LevelUpSound);
	}
}
