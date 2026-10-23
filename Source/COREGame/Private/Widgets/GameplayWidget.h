// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GAS/CGameplayAbilityTypes.h"
#include "GameplayWidget.generated.h"

class FGameChatClickAwayProcessor;

/**
 * 
 */
UCLASS()
class UGameplayWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void ConfigureAbilities(const TMap<ECAbilityInputID, TSubclassOf<UGameplayAbility>>& Abilities);
	void ToggleShop();

	UFUNCTION()
	void ToggleGameplayMenu(); 

	void ShowGameplayMenu();
	void SetGameplayMenuTitle(const FString& NewTitle);

	void FocusChatInput();

	bool HandleGameViewportClicked(const FPointerEvent& MouseEvent);

	void ShowScoreboard();
	void HideScoreboard();

	void ShowHoldProgress(float MaxHoldTime, bool bCountDown = false);
	void HideHoldProgress();

	void EnterMatchFinishedState();

private:
	UPROPERTY(meta = (BindWidget))
	class UValueGauge* HealthBar;
	
	UPROPERTY(meta = (BindWidget))
	class UValueGauge* ManaBar;

	UPROPERTY(meta = (BindWidget))
	class UStatsGauge* MoveSpeedGauge;

	UPROPERTY(meta = (BindWidget))
	class UStatsGauge* ArmorGauge;

	UPROPERTY(meta = (BindWidget))
	class UStatsGauge* MagicResistanceGauge;

	UPROPERTY(meta = (BindWidget))
	class UStatsGauge* LifeStealGauge;

	UPROPERTY(meta = (BindWidget))
	class UStatsGauge* IntelligenceGauge;

	UPROPERTY(meta = (BindWidget))
	class UStatsGauge* StrengthGauge;

	UPROPERTY(meta = (BindWidget))
	class UAbilityListView* AbilityListView;

	UPROPERTY(meta = (BindWidget))
	class UUltimateAbilityGauge* UltimateGauge;

	UPROPERTY(meta = (BindWidget))
	class UShopWidget* ShopWidget;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	class UWidgetAnimation* ShopPopupAnimation;

	void PlayShopPopupAnimation(bool bPlayForward);
	void SetOwningPawnInputEnabled(bool bPawnInputEnabled);
	void SetShowMouseCursor(bool bShowMouseCursor);
	void SetFocusToGameAndUI();
	void SetFocusToGameOnly();

	UPROPERTY()
	class UAbilitySystemComponent* OwnerAbilitySystemComponent;

	UPROPERTY(meta = (BindWidget))
	class UInventoryWidget* InventoryWidget;

	UPROPERTY(meta = (BindWidget))
	class USkeletalMeshRenderWidget* HeadshotWidget;
	
	UPROPERTY(meta = (BindWidget))
	class UMatchStatWidget* MatchStatWidget;

	UPROPERTY(meta = (BindWidget))
	class UGameplayMenu* GameplayMenu;

	UPROPERTY(meta = (BindWidget))
	class UWidgetSwitcher* MainSwitcher;

	UPROPERTY(meta = (BindWidget))
	class UCrosshairWidget* CrosshairWidget;

	UPROPERTY(meta = (BindWidget))
	class UCanvasPanel* GameplayWidgetRootPanel;

	UPROPERTY(meta = (BindWidget))
	class UCanvasPanel* GameplayMenuRootPanel;

	UPROPERTY(meta = (BindWidget))
	class UCanvasPanel* QuitCheckWidgetRoot;

	UPROPERTY(meta = (BindWidget))
	class UButton* QuitConfirmBtn;

	UPROPERTY(meta = (BindWidget))
	class UButton* QuitCancelBtn;

	UFUNCTION()
	void QuitConfirmBtnClicked();

	UFUNCTION()
	void QuitCancelBtnClicked();

	UFUNCTION()
	void QuitMatchBtnClicked();

	UPROPERTY(meta=(BindWidget))
	class UChatWidget* ChatWidget;

	void OnChatInputFinished();
	void OnChatInputRequested();

	bool IsNonGameplayUIOpen() const;

	TSharedPtr<FGameChatClickAwayProcessor> ChatClickAwayProcessor;

	UPROPERTY(meta=(BindWidget))
	class UKillFeedWidget* KillFeedWidget;

	UPROPERTY(meta = (BindWidget))
	class UScoreboardWidget* ScoreboardWidget;

	FTimerHandle ScoreboardRefreshTimerHandle;

	UPROPERTY(meta = (BindWidget))
	class UBuffTrayWidget* BuffTrayWidget;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* MatchTimerText;

	FTimerHandle MatchTimerRefreshHandle;
	
	void RefreshMatchTimer();

	UPROPERTY(meta = (BindWidget))
	class UHoldProgressWidget* HoldProgressWidget;

	UPROPERTY(meta=(BindWidget))
	class UOverlay* GoldPopupAnchor;

	UPROPERTY(EditDefaultsOnly, Category = "Gold")
	TSubclassOf<class UGoldPopupWidget> GoldPopupWidgetClass;

	UPROPERTY()
	class UGoldPopupWidget* ActiveGoldPopup;

	float LastKnownGold = 0.f;

	void GoldValueChanged(const FOnAttributeChangeData& Data);
	void OnGoldPopupFinished();

	UPROPERTY(meta = (BindWidget))
	class UBorder* RespawnOverlay;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* RespawnTimerText;

	FTimerHandle RespawnCheckTimerHandle;
	void RefreshRespawnOverlay();

	UPROPERTY(meta = (BindWidget))
	class UBorder* MatchStartCountdownOverlay;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* MatchStartCountdownText;

	UPROPERTY(EditDefaultsOnly, Category = "Match Start")
	FText MatchStartAnnounceFormat = INVTEXT("게임이 {0}초 뒤 시작됩니다");

	UPROPERTY(EditDefaultsOnly, Category = "Match Start")
	FText MatchStartText = INVTEXT("게임이 시작되었습니다");

	UPROPERTY(EditDefaultsOnly, Category = "Match Start")
	float MatchStartTextDuration = 0.5f;

	FTimerHandle MatchStartCountdownTimerHandle;
	void RefreshMatchStartCountdown();

	UPROPERTY(meta = (BindWidget))
	class UAmmoWidget* AmmoWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Sound")
	class USoundBase* LevelUpSound;

	void LevelChanged(const struct FOnAttributeChangeData& Data);

	bool bMatchFinished = false;

	// 첫 미니언 웨이브 몇 초 전에 안내할지
	UPROPERTY(EditDefaultsOnly, Category = "Match Start|Minion")
	float MinionWarningLeadTime = 30.f;

	// {0} = 미니언 생성 시간 대체 문자. 꼭 적어야함
	UPROPERTY(EditDefaultsOnly, Category = "Match Start|Minion")
	FText MinionWarningFormat = INVTEXT("미니언 생성까지 {0}초 남았습니다");

	UPROPERTY(EditDefaultsOnly, Category = "Match Start|Minion")
	FText MinionSpawnedText = INVTEXT("미니언이 생성되었습니다");

	// 미니언 안내 문구 표시 시간
	UPROPERTY(EditDefaultsOnly, Category = "Match Start|Minion")
	float MinionNoticeDuration = 3.f;
};
