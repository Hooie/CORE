// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player/ChatTypes.h"
#include "Framework/Application/IInputProcessor.h"
#include "ChatWidget.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnChatInputFinished);
DECLARE_MULTICAST_DELEGATE(FOnChatInputRequested);

class FChatFocusInputProcessor : public IInputProcessor
{
public:
	FChatFocusInputProcessor(TWeakObjectPtr<class UChatWidget> InOwner) : Owner(InOwner) {}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;

private:
	TWeakObjectPtr<class UChatWidget> Owner;
};

UCLASS()
class UChatWidget : public UUserWidget 
{
	GENERATED_BODY()
	
public:
	FOnChatInputFinished OnChatInputFinished;
	FOnChatInputRequested OnChatInputRequested;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	//FChatFocusInputProcessor로 이 함수 대체
	//virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	bool IsInputFocused() const;
	void FocusInput();

	void FocusInputWithChannel(EChatChannel Channel);
	FGeometry GetChatInputGeometry() const;

private:
	UPROPERTY(meta=(BindWidget))
	class UScrollBox* ChatScrollBox;

	UPROPERTY(meta = (BindWidget))
	class UEditableTextBox* ChatInputText;

	UPROPERTY(meta = (BindWidget))
	class UButton* SendBtn;

	UPROPERTY(meta = (BindWidget))
	class UButton* TeamChannelBtn;

	UPROPERTY(meta = (BindWidget))
	class UButton* AllChannelBtn;

	EChatChannel SelectedChannel = EChatChannel::Team;

	UPROPERTY()
	class UChatComponent* ChatComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Chat")
	FSlateFontInfo ChatFont;

	UPROPERTY(EditDefaultsOnly, Category = "Chat")
	FLinearColor TeamChatColor = FLinearColor(0.3f, 0.5f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = "Chat")
	FLinearColor EnemyAllChatColor = FLinearColor(1.f, 0.3f, 0.3f);

	UPROPERTY(EditDefaultsOnly, Category = "Chat")
	FLinearColor SelectedChannelBtnColor = FLinearColor(0.f, 0.7f, 0.7f);

	UPROPERTY(EditDefaultsOnly, Category = "Chat")
	FLinearColor DefaultChannelBtnColor = FLinearColor::White;

	void TryScrollToEnd();
	int32 PendingScrollRetries = 0;
	bool bScrollRetryActive = false;

	void OnChatMessageReceived(const FChatMessage& Message);
	FString FormatMessage(const FChatMessage& Message) const;
	FLinearColor GetMessageColor(const FChatMessage& Message) const;

	UFUNCTION()
	void SendBtnClicked();

	UFUNCTION()
	void ChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void TeamChannelBtnClicked();

	UFUNCTION()
	void AllChannelBtnClicked();

	void UpdateChannelButtonVisuals();

	TSharedPtr<FChatFocusInputProcessor> ChatFocusInputProcessor;
	
};
