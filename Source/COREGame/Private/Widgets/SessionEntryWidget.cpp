// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/SessionEntryWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void USessionEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SessionBtn->OnClicked.AddDynamic(this, &USessionEntryWidget::SessionEntrySelected);
}

void USessionEntryWidget::InitializeEntry(const FString& Name, const FString& SessionIdStr, const FString& HostNickname, int32 CurrentPlayerCount, int32 MaxPlayerCount)
{
	SessionNameText->SetText(FText::FromString(Name));
	CachedSessionIdStr = SessionIdStr;
	SessionHostNicknameText->SetText(FText::FromString(HostNickname));

	FString PlayerCountStr = FString::Printf(TEXT(" (%d / %d)"), CurrentPlayerCount, MaxPlayerCount);
	SessionPlayerCountText->SetText(FText::FromString(PlayerCountStr));
}

void USessionEntryWidget::SessionEntrySelected()
{
	OnSessionEntrySelected.Broadcast(CachedSessionIdStr);
}
