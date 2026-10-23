// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "OnlineSessionSettings.h"
#include "MainMenuWidget.generated.h"

/**
 * 
 */
UCLASS()
class UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;

	/**************************************/
	/*                Main                */
	/**************************************/
private:
	UPROPERTY(meta = (BindWidget))
	class UWidgetSwitcher* MainSwitcher;

	UPROPERTY()
	class UCGameInstance* CGameInstance;

	UFUNCTION()
	void SwitchToMainWidget();

	UPROPERTY(meta = (BindWidget))
	class UWidget* MainWidgetRoot;

	UPROPERTY(meta=(BindWidget))
	UButton* FindGameBtn;

	UPROPERTY(meta = (BindWidget))
	UButton* ConfigBtn;

	UPROPERTY(meta = (BindWidget))
	UButton* QuitGameBtn;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* PlayerNicknameText;

	UPROPERTY(meta = (BindWidget))
	UWidget* QuitCheckWidgetRoot;

	UPROPERTY(meta = (BindWidget))
	UButton* QuitConfirmBtn;

	UPROPERTY(meta = (BindWidget))
	UButton* QuitCancelBtn;

	UPROPERTY(meta = (BindWidget))
	UButton* LogoutBtn;

	UFUNCTION()
	void ConfigBtnClicked();
	
	UFUNCTION()
	void QuitGameBtnClicked();

	UFUNCTION()
	void QuitConfirmBtnClicked();
	
	UFUNCTION()
	void ReturnToMainWidget();

	UFUNCTION()
	void LogoutBtnClicked();

	void LogoutCompleted(bool bWasSuccessful);

	UPROPERTY(meta=(BindWidget))
	class UBorder* ErrorMessageTextBorder;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* ErrorMessageText;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	class UWidgetAnimation* ErrorMessagePopupAnimation;

	void PlayErrorMessagePopupAnimation(bool bPlayForward);

	/**************************************/
	/*               config               */
	/**************************************/
	UPROPERTY(meta = (BindWidget))
	class UGameConfigWidget* ConfigWidgetRoot;

	/**************************************/
	/*              Session               */
	/**************************************/
	UPROPERTY(meta = (BindWidget))
	class UWidget* SessionWidgetRoot;

	UPROPERTY(meta = (BindWidget))
	UButton* FromSessionToMainWidgetBtn;

	UPROPERTY(meta = (BindWidget))
	UButton* CreateSessionBtn;

	UPROPERTY(meta = (BindWidget))
	class UEditableText* NewSessionNameText;

	UPROPERTY(EditDefaultsOnly, Category = "Session")
	int32 MaxSessionNameLength = 20;

	UFUNCTION()
	void SwitchToSessionWidget();

	UFUNCTION()
	void NewSessionTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void CreateSessionBtnClicked();

	UFUNCTION()
	void NewSessionTextChanged(const FText& NewText);

	UFUNCTION()
	void CancelSessionCreation();

	void JoinSessionFailed();

	void JoinRejected(const FString& RejectReason);

	void UpdateLobbyList(const TArray<FOnlineSessionSearchResult>& SearchResults);

	UPROPERTY(meta=(BindWidget))
	class UScrollBox* SessionScrollBox;

	UPROPERTY(meta = (BindWidget))
	class UButton* JoinSessionBtn;

	UPROPERTY(EditDefaultsOnly, Category = "Session")
	TSubclassOf<class USessionEntryWidget> SessionEntryClass;

	void SessionEntrySelected(const FString& SelectedSessionIdStr);

	FString CurrentSelectedSessionId = "";

	UFUNCTION()
	void JoinSessionBtnClicked();

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* InSessionPlayerNicknameText;

	UPROPERTY(meta = (BindWidget))
	class UButton* RefreshSessionBtn;

	FTimerHandle RefreshCooldownTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Session")
	float RefreshSessionCooldownDuration = 2.f;

	UFUNCTION()
	void RefreshSessionBtnClicked();

	void EnableRefreshSessionBtn();

	/**************************************/
	/*               Login                */
	/**************************************/
private:
	UPROPERTY(meta = (BindWidget))
	class UWidget* LoginWidgetRoot;

	UPROPERTY(meta = (BindWidget))
	class UButton* LoginQuitGameBtn;

	UPROPERTY(meta = (BindWidget))
	class UButton* LoginBtn;

	UPROPERTY(meta = (BindWidget))
	UWidget* LogoutCheckWidgetRoot;

	UPROPERTY(meta = (BindWidget))
	UButton* LogoutConfirmBtn;

	UPROPERTY(meta = (BindWidget))
	UButton* LogoutCancelBtn;

	UPROPERTY(EditDefaultsOnly, Category = "Login")
	float LoginTimeoutDuration = 60.f;

	FTimerHandle LoginTimeoutTimerHandle;

	void LoginTimedOut();

	UFUNCTION()
	void LoginBtnClicked();

	void LoginCompleted(bool bWasSuccessful, const FString& PlayerNickname, const FString& ErrorMsg);

	UFUNCTION()
	void LogoutConfirmBtnClicked();

	/**************************************/
	/*              Waiting               */
	/**************************************/
private:
	UPROPERTY(meta = (BindWidget))
	class UWaitingWidget* WaitingWidget;

	FOnButtonClickedEvent& SwitchToWaitWidget(const FText& WaitInfo, bool bAllowCancel = false);
};
