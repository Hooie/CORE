// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player/KillFeedTypes.h"
#include "KillFeedWidget.generated.h"

/**
 * 
 */
UCLASS()
class UKillFeedWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* KillFeedContainer;

	UPROPERTY(EditDefaultsOnly, Category = "Kill Feed")
	TSubclassOf<class UKillFeedEntryWidget> KillFeedEntryWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Kill Feed")
	float EntryDisplayDuration = 5.f;
	
	void OnkillFeedEntryReceived(const FKillFeedEntry& Entry);
	void RemoveEntry(TWeakObjectPtr<class UKillFeedEntryWidget> EntryWidget);
};
