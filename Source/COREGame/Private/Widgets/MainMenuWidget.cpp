// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/MainMenuWidget.h"
#include "Components/Border.h"
#include "Components/EditableText.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Framework/CGameInstance.h"
#include "Network/CNetStatics.h"
#include "Widgets/GameConfigWidget.h"
#include "Widgets/SessionEntryWidget.h"
#include "Widgets/WaitingWidget.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameplayWidget.h"

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogTemp, Warning, TEXT("MainMenuWidget::NativeConstruct called"));

	ErrorMessageTextBorder->SetVisibility(ESlateVisibility::Hidden);
	ErrorMessageText->SetVisibility(ESlateVisibility::Hidden);

	CGameInstance = GetGameInstance<UCGameInstance>();
	if (CGameInstance)
	{
		CGameInstance->OnLoginCompleted.AddUObject(this, &UMainMenuWidget::LoginCompleted); 
		CGameInstance->OnLogoutCompleted.AddUObject(this, &UMainMenuWidget::LogoutCompleted);

		if (CGameInstance->IsLoggedIn())
		{
			LoginCompleted(true, CGameInstance->GetCachedPlayerNickname(), TEXT(""));
		}

		CGameInstance->OnJoinSessionFailed.AddUObject(this, &UMainMenuWidget::JoinSessionFailed);
		CGameInstance->OnGlobalSessionSearchCompleted.AddUObject(this, &UMainMenuWidget::UpdateLobbyList);
		CGameInstance->OnReturnToMainMenu.AddUObject(this, &UMainMenuWidget::SwitchToSessionWidget);
		CGameInstance->OnJoinRejected.AddUObject(this, &UMainMenuWidget::JoinRejected);
		CGameInstance->StartGlobalSessionSearch();

		FString PendingReason = CGameInstance->ConsumePendingJoinRejectReason();
		if ( !PendingReason.IsEmpty() )
		{
			JoinRejected(PendingReason);
		}
	}

	LoginBtn->OnClicked.AddDynamic(this, &UMainMenuWidget::LoginBtnClicked);
	LoginQuitGameBtn->OnClicked.AddDynamic(this, &UMainMenuWidget::QuitGameBtnClicked);

	FindGameBtn->OnClicked.AddDynamic(this, &UMainMenuWidget::SwitchToSessionWidget);
	ConfigBtn->OnClicked.AddDynamic(this, &UMainMenuWidget::ConfigBtnClicked);
	QuitGameBtn->OnClicked.AddDynamic(this, &UMainMenuWidget::QuitGameBtnClicked);

	LogoutBtn->OnClicked.AddDynamic(this, &UMainMenuWidget::LogoutBtnClicked);
	LogoutConfirmBtn->OnClicked.AddDynamic(this, &UMainMenuWidget::LogoutConfirmBtnClicked);
	LogoutCancelBtn->OnClicked.AddDynamic(this, &UMainMenuWidget::ReturnToMainWidget);

	QuitConfirmBtn->OnClicked.AddDynamic(this, &UMainMenuWidget::QuitConfirmBtnClicked);
	QuitCancelBtn->OnClicked.AddDynamic(this, &UMainMenuWidget::ReturnToMainWidget);

	CreateSessionBtn->OnClicked.AddDynamic(this, &UMainMenuWidget::CreateSessionBtnClicked);
	CreateSessionBtn->SetIsEnabled(false);

	ConfigWidgetRoot->OnReturnBtnClicked.AddUObject(this, &UMainMenuWidget::ReturnToMainWidget);
	FromSessionToMainWidgetBtn->OnClicked.AddDynamic(this, &UMainMenuWidget::ReturnToMainWidget);

	NewSessionNameText->OnTextChanged.AddDynamic(this, &UMainMenuWidget::NewSessionTextChanged);
	NewSessionNameText->OnTextCommitted.AddDynamic(this, &UMainMenuWidget::NewSessionTextCommitted);

	JoinSessionBtn->OnClicked.AddDynamic(this, &UMainMenuWidget::JoinSessionBtnClicked);
	JoinSessionBtn->SetIsEnabled(false);

	RefreshSessionBtn->OnClicked.AddDynamic(this, &UMainMenuWidget::RefreshSessionBtnClicked);
}

void UMainMenuWidget::SwitchToMainWidget()
{
	if (MainSwitcher)
	{
		MainSwitcher->SetActiveWidget(MainWidgetRoot);
	}
}

void UMainMenuWidget::ConfigBtnClicked()
{
	if ( ConfigWidgetRoot )
	{
		ConfigWidgetRoot->InitializeSettings();
		MainSwitcher->SetActiveWidget(ConfigWidgetRoot);
	}
}

void UMainMenuWidget::QuitGameBtnClicked()
{
	if(QuitCheckWidgetRoot )
	{
		MainSwitcher->SetActiveWidget(QuitCheckWidgetRoot);
	}
}

void UMainMenuWidget::QuitConfirmBtnClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, true);
}

void UMainMenuWidget::ReturnToMainWidget()
{
	if ( MainWidgetRoot )
	{
		MainSwitcher->SetActiveWidget(MainWidgetRoot);
	}
}

void UMainMenuWidget::LogoutBtnClicked()
{
	if ( LogoutCheckWidgetRoot )
	{
		MainSwitcher->SetActiveWidget(LogoutCheckWidgetRoot);
	}
}

void UMainMenuWidget::LogoutCompleted(bool bWasSuccessful)
{
	if ( bWasSuccessful )
	{
		PlayerNicknameText->SetText(FText::GetEmpty());
		InSessionPlayerNicknameText->SetText(FText::GetEmpty());
		MainSwitcher->SetActiveWidget(LoginWidgetRoot);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Logout Failed, staying on Main Widget"));
		MainSwitcher->SetActiveWidget(MainWidgetRoot);
	}
}

void UMainMenuWidget::PlayErrorMessagePopupAnimation(bool bPlayForward)
{
	if ( bPlayForward )
	{
		ErrorMessageTextBorder->SetVisibility(ESlateVisibility::Visible); 
		ErrorMessageText->SetVisibility(ESlateVisibility::Visible);

		PlayAnimationForward(ErrorMessagePopupAnimation);
	}
	else
	{
		PlayAnimationReverse(ErrorMessagePopupAnimation);
	}
}

void UMainMenuWidget::SwitchToSessionWidget()
{
	if ( SessionWidgetRoot )
	{
		MainSwitcher->SetActiveWidget(SessionWidgetRoot);

		if ( CGameInstance )
		{
			CGameInstance->RefreshSessionsNow();
		}
	}
}

void UMainMenuWidget::NewSessionTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	FString SessionName = Text.ToString();
	if ( SessionName.Len() > MaxSessionNameLength )
	{
		SessionName = SessionName.Left(MaxSessionNameLength);
		NewSessionNameText->SetText(FText::FromString(SessionName));
	}

	CreateSessionBtn->SetIsEnabled(!SessionName.IsEmpty());
}

void UMainMenuWidget::CreateSessionBtnClicked()
{
	if (CGameInstance && CGameInstance->IsLoggedIn())
	{
		CGameInstance->RequestCreateAndJoinSession(FName(NewSessionNameText->GetText().ToString()));
		SwitchToWaitWidget(FText::FromString(TEXT("로비 생성중")), true).AddDynamic(this, &UMainMenuWidget::CancelSessionCreation);
	}
}

void UMainMenuWidget::NewSessionTextChanged(const FText& NewText)
{
	FString SessionName = NewText.ToString();

	FString AllowedSpecialChars = TEXT("!?()~^@#&+-=*\'\"<>/");

	FString FilteredName;
	for ( TCHAR Char : SessionName )
	{
		bool bIsEnglish = (Char >= 'A' && Char <= 'Z') || (Char >= 'a' && Char <= 'z');
		bool bIsKorean = Char >= 0xAC00 && Char <= 0xD7A3;
		bool bIsKoreanJamo = Char >= 0x3130 && Char <= 0x318F;
		bool bIsKoreanLead = Char >= 0x1100 && Char <= 0x11FF;
		bool bIsDigit = Char >= '0' && Char <= '9';
		bool bIsSpace = Char == TEXT(' ');
		bool bIsAllowedSpecial = AllowedSpecialChars.Contains(FString(1, &Char));

		if ( bIsEnglish || bIsKorean || bIsKoreanJamo || bIsKoreanLead || bIsDigit || bIsSpace || bIsAllowedSpecial )
			FilteredName.AppendChar(Char);
	}

	if ( FilteredName != SessionName )
		NewSessionNameText->SetText(FText::FromString(FilteredName));

	if ( FilteredName.TrimStartAndEnd().IsEmpty() && !FilteredName.IsEmpty() )
	{
		NewSessionNameText->SetText(FText::GetEmpty());
		CreateSessionBtn->SetIsEnabled(false);
		return;
	}

	CreateSessionBtn->SetIsEnabled(!FilteredName.TrimStartAndEnd().IsEmpty());
}

void UMainMenuWidget::CancelSessionCreation()
{
	if (CGameInstance)
	{
		CGameInstance->CancelSessionCreation();
	}

	SwitchToSessionWidget();
}

void UMainMenuWidget::JoinSessionFailed()
{
	SwitchToSessionWidget();
}

void UMainMenuWidget::JoinRejected(const FString& RejectReason)
{
	UE_LOG(LogTemp, Warning, TEXT("Join Rejected : %s"), *RejectReason);

	if ( ErrorMessageText )
	{
		ErrorMessageText->SetText(FText::FromString(RejectReason));

		PlayErrorMessagePopupAnimation(true);
	}

	if ( CGameInstance )
	{
		CGameInstance->ConsumePendingJoinRejectReason();
	}

	SwitchToSessionWidget();
}

void UMainMenuWidget::UpdateLobbyList(const TArray<FOnlineSessionSearchResult>& SearchResults)
{
	UE_LOG(LogTemp, Warning, TEXT("Updating Session search Results"));
	SessionScrollBox->ClearChildren();

	bool bCurrentSelectedSessionValid = false;
	for (const FOnlineSessionSearchResult& SearchResult : SearchResults)
	{
		USessionEntryWidget* NewSessionEntryWidget = CreateWidget<USessionEntryWidget>(GetOwningPlayer(), SessionEntryClass);
		if (NewSessionEntryWidget)
		{
			FString SessionName = "Name_None";
			SearchResult.Session.SessionSettings.Get<FString>(UCNetStatics::GetSessionNameKey(), SessionName);

			SessionName = FGenericPlatformHttp::UrlDecode(SessionName);

			if ( SessionName.IsEmpty() || SessionName == TEXT("Name_None") )
				continue;

			FString SessionIdStr = SearchResult.Session.GetSessionIdStr();
			
			FString HostNickname = TEXT("Unknown");
			SearchResult.Session.SessionSettings.Get<FString>(UCNetStatics::GetHostNicknameKey(), HostNickname);
			
			HostNickname = FGenericPlatformHttp::UrlDecode(HostNickname);

			int32 MaxPlayerCount = SearchResult.Session.SessionSettings.NumPublicConnections;
			int64 CurrentPlayerCount64 = 0;
			if ( const FOnlineSessionSetting* PlayerCountSetting = SearchResult.Session.SessionSettings.Settings.Find(UCNetStatics::GetPlayerCountKey()) )
			{
				PlayerCountSetting->Data.GetValue(CurrentPlayerCount64);
			}
			int32 CurrentPlayerCount = static_cast< int32 >(CurrentPlayerCount64);

			int64 HeroSelectionStarted64 = 0;
			if ( const FOnlineSessionSetting* PlayerCountSetting = SearchResult.Session.SessionSettings.Settings.Find(UCNetStatics::GetHeroSelectionStartedKey()) )
			{
				PlayerCountSetting->Data.GetValue(HeroSelectionStarted64);
			}

			if ( HeroSelectionStarted64 != 0 )
				continue;

			NewSessionEntryWidget->InitializeEntry(SessionName, SessionIdStr, HostNickname, CurrentPlayerCount, MaxPlayerCount);
			NewSessionEntryWidget->OnSessionEntrySelected.AddUObject(this, &UMainMenuWidget::SessionEntrySelected);

			SessionScrollBox->AddChild(NewSessionEntryWidget);

			if (CurrentSelectedSessionId == SessionIdStr)
			{
				bCurrentSelectedSessionValid = true;
			}
		}
	}

	CurrentSelectedSessionId = bCurrentSelectedSessionValid ? CurrentSelectedSessionId : "";

	JoinSessionBtn->SetIsEnabled(bCurrentSelectedSessionValid);
}

void UMainMenuWidget::SessionEntrySelected(const FString& SelectedSessionIdStr)
{
	CurrentSelectedSessionId = SelectedSessionIdStr;
}

void UMainMenuWidget::JoinSessionBtnClicked()
{
	if ( !CGameInstance || CurrentSelectedSessionId.IsEmpty() )
	{
		UE_LOG(LogTemp, Warning, TEXT("Can't join Session, no session Selected"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Trying to join session with id: %s"), *CurrentSelectedSessionId);

	if ( CGameInstance->JoinSessionWithId(CurrentSelectedSessionId) )
		SwitchToWaitWidget(FText::FromString(TEXT("게임에 참가하는 중")), false);
}

void UMainMenuWidget::RefreshSessionBtnClicked()
{
	if ( CGameInstance )
	{
		CGameInstance->RefreshSessionsNow();
	}

	RefreshSessionBtn->SetIsEnabled(false);
	GetWorld()->GetTimerManager().SetTimer(RefreshCooldownTimerHandle, this, &UMainMenuWidget::EnableRefreshSessionBtn, RefreshSessionCooldownDuration, false);
}

void UMainMenuWidget::EnableRefreshSessionBtn()
{
	RefreshSessionBtn->SetIsEnabled(true);
}

void UMainMenuWidget::LoginTimedOut()
{
	UE_LOG(LogTemp, Warning, TEXT("Login Timed Out! (%.0f초 경과, 콜백 없음)"), LoginTimeoutDuration);

	LoginCompleted(false, TEXT(""), TEXT("로그인 시간이 초과되었습니다. 다시 시도해주세요."));
}

void UMainMenuWidget::LoginBtnClicked()
{
	UE_LOG(LogTemp, Warning, TEXT("Loging in !!"));

	if ( CGameInstance && !CGameInstance->IsLoggedIn() && !CGameInstance->IsLoggingIn() )
	{
		CGameInstance->ClientAccountPortalLogin();

		SwitchToWaitWidget(FText::FromString(TEXT("로그인 중...")));

		GetWorld()->GetTimerManager().ClearTimer(LoginTimeoutTimerHandle);
		GetWorld()->GetTimerManager().SetTimer(LoginTimeoutTimerHandle, this, &UMainMenuWidget::LoginTimedOut, LoginTimeoutDuration, false);
	}
}

void UMainMenuWidget::LoginCompleted(bool bWasSuccessful, const FString& PlayerNickname, const FString& ErrorMsg)
{
	GetWorld()->GetTimerManager().ClearTimer(LoginTimeoutTimerHandle);

	if ( bWasSuccessful )
	{
		UE_LOG(LogTemp, Warning, TEXT("Login Successful !!"))

			FString WelcomingStr = PlayerNickname + TEXT("님 환영합니다.");
		FString SessionNicknameStr = TEXT("현재 접속 중인 계정: ") + PlayerNickname;

		PlayerNicknameText->SetText(FText::FromString(WelcomingStr));
		InSessionPlayerNicknameText->SetText(FText::FromString(SessionNicknameStr));
		SwitchToMainWidget();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Login Failed !! Error Msg: %s"), *ErrorMsg);

		MainSwitcher->SetActiveWidget(LoginWidgetRoot);
	}
}

void UMainMenuWidget::LogoutConfirmBtnClicked()
{
	if ( CGameInstance )
	{
		CGameInstance->Logout();
		SwitchToWaitWidget(FText::FromString(TEXT("로그아웃 중...")));
	}
}

FOnButtonClickedEvent& UMainMenuWidget::SwitchToWaitWidget(const FText& WaitInfo, bool bAllowCancel)
{
	MainSwitcher->SetActiveWidget(WaitingWidget);
	WaitingWidget->SetWaitInfo(WaitInfo, bAllowCancel);

	return WaitingWidget->ClearAndGetButtonClickedEvent();
}
