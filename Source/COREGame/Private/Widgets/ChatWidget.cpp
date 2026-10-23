// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/ChatWidget.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Framework/Application/SlateApplication.h"
#include "Player/ChatComponent.h"

bool FChatFocusInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if ( InKeyEvent.GetKey() != EKeys::Enter )
		return false;

	if ( InKeyEvent.IsAltDown() || InKeyEvent.IsControlDown() )
		return false;

	if ( !Owner.IsValid() || Owner->IsInputFocused() )
		return false;

	EChatChannel Channel = InKeyEvent.IsShiftDown() ? EChatChannel::All : EChatChannel::Team;
	Owner->FocusInputWithChannel(Channel);

	return true;
}

void UChatWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if(APlayerController* PC = GetOwningPlayer() )
	{
		ChatComponent = PC->FindComponentByClass<UChatComponent>();
	}

	if ( ChatComponent )
	{
		ChatComponent->OnChatMessageReceived.AddUObject(this, &UChatWidget::OnChatMessageReceived);
	}

	SendBtn->OnClicked.AddDynamic(this, &UChatWidget::SendBtnClicked);
	ChatInputText->OnTextCommitted.AddDynamic(this, &UChatWidget::ChatInputTextCommitted);

	TeamChannelBtn->OnClicked.AddDynamic(this, &UChatWidget::TeamChannelBtnClicked);
	AllChannelBtn->OnClicked.AddDynamic(this, &UChatWidget::AllChannelBtnClicked);

	UpdateChannelButtonVisuals();

	SetIsFocusable(true);

	ChatFocusInputProcessor = MakeShared<FChatFocusInputProcessor>(TWeakObjectPtr<UChatWidget>(this));
	FSlateApplication::Get().RegisterInputPreProcessor(ChatFocusInputProcessor);
}

void UChatWidget::NativeDestruct()
{
	if ( ChatFocusInputProcessor.IsValid() )
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(ChatFocusInputProcessor);
		ChatFocusInputProcessor.Reset();
	}

	Super::NativeDestruct();
}

//FReply UChatWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
//{
//	if ( InKeyEvent.GetKey() == EKeys::Enter && ChatInputText && !ChatInputText->HasKeyboardFocus() )
//	{
//		ChatInputText->SetKeyboardFocus();
//		return FReply::Handled();
//	}
//	
//	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
//}

bool UChatWidget::IsInputFocused() const
{
	return ChatInputText && ChatInputText->HasKeyboardFocus();
}

void UChatWidget::FocusInput()
{
	if ( ChatInputText )
	{
		ChatInputText->SetKeyboardFocus();
	}
}

void UChatWidget::FocusInputWithChannel(EChatChannel Channel)
{
	SelectedChannel = Channel;
	UpdateChannelButtonVisuals();

	OnChatInputRequested.Broadcast();

	FocusInput();
}

FGeometry UChatWidget::GetChatInputGeometry() const
{
	return ChatInputText ? ChatInputText->GetCachedGeometry() : FGeometry();
}

void UChatWidget::TryScrollToEnd()
{
	ChatScrollBox->ScrollToEnd();

	if ( --PendingScrollRetries > 0 )
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UChatWidget::TryScrollToEnd);
	}
	else
	{
		bScrollRetryActive = false;
	}
}

void UChatWidget::OnChatMessageReceived(const FChatMessage& Message)
{
	FString Formatted = FormatMessage(Message);

	UTextBlock* NewLine = NewObject<UTextBlock>(this);
	NewLine->SetText(FText::FromString(Formatted));
	NewLine->SetFont(ChatFont);
	NewLine->SetAutoWrapText(true);
	NewLine->SetColorAndOpacity(FSlateColor(GetMessageColor(Message)));

	ChatScrollBox->AddChild(NewLine);
	
	PendingScrollRetries = 5;

	if ( !bScrollRetryActive )
	{
		bScrollRetryActive = true;
		TryScrollToEnd();
	}
}

FString UChatWidget::FormatMessage(const FChatMessage& Message) const
{
	FString ChannelLabel = (Message.Channel == EChatChannel::Team) ? TEXT("팀") : TEXT("전체");

	return FString::Printf(TEXT("(%s)[%02d:%02d:%02d] %s : %s"),
		*ChannelLabel,
		Message.Hour, Message.Minute, Message.Second,
		*Message.SenderNickname,
		*Message.MessageText
	);
}

FLinearColor UChatWidget::GetMessageColor(const FChatMessage& Message) const
{
	if ( Message.Channel == EChatChannel::Team )
	{
		return TeamChatColor;
	}

	uint8 CurrentLocalTeamId = ChatComponent ? ChatComponent->GetOwnerTeamId() : 0;

	return (Message.SenderTeamId == CurrentLocalTeamId) ? TeamChatColor : EnemyAllChatColor;
}

void UChatWidget::SendBtnClicked()
{
	FString Text = ChatInputText->GetText().ToString();
	if ( Text.IsEmpty() || !ChatComponent )
		return;

	ChatComponent->Server_SendChatMessage(Text, SelectedChannel);
	ChatInputText->SetText(FText::GetEmpty());
}

void UChatWidget::ChatInputTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if ( CommitMethod == ETextCommit::OnEnter )
	{
		SendBtnClicked();
		OnChatInputFinished.Broadcast();
	}
	else if(CommitMethod == ETextCommit::OnCleared )
	{
		ChatInputText->SetText(FText::GetEmpty());
		OnChatInputFinished.Broadcast();
	}
}

void UChatWidget::TeamChannelBtnClicked()
{
	if ( SelectedChannel == EChatChannel::Team )
		return;

	SelectedChannel = EChatChannel::Team;
	UpdateChannelButtonVisuals();
	
	ChatInputText->SetKeyboardFocus();
}

void UChatWidget::AllChannelBtnClicked()
{
	if ( SelectedChannel == EChatChannel::All )
		return;

	SelectedChannel = EChatChannel::All;
	UpdateChannelButtonVisuals();

	ChatInputText->SetKeyboardFocus();
}

void UChatWidget::UpdateChannelButtonVisuals()
{
	bool bTeamSelected = (SelectedChannel == EChatChannel::Team);

	TeamChannelBtn->SetIsEnabled(!bTeamSelected);
	AllChannelBtn->SetIsEnabled(bTeamSelected);

	/*TeamChannelBtn->SetBackgroundColor(bTeamSelected ? SelectedChannelBtnColor : DefaultChannelBtnColor);
	AllChannelBtn->SetBackgroundColor(bTeamSelected ? DefaultChannelBtnColor : SelectedChannelBtnColor);*/
}