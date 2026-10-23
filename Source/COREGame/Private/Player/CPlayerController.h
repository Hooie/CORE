// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GenericTeamAgentInterface.h"

#include "CPlayerController.generated.h"

class ACPlayerCharacter;
class UGameplayWidget;
class UInputMappingContext;
class UInputAction;


/**
 * 
 */
UCLASS()
class ACPlayerController : public APlayerController, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	ACPlayerController();

	// ���������� ȣ���
	void OnPossess(APawn* NewPawn) override;
	// Ŭ���̾�Ʈ������ ȣ��ǰ�, ������ ����(P2P)������ ȣ���.
	void AcknowledgePossession(APawn* NewPawn) override;

	/** Assigns Team Agent to given TeamID */
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;

	/** Retrieve team identifier in form of FGenericTeamId */
	virtual FGenericTeamId GetGenericTeamId() const override;

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	virtual void SetupInputComponent() override;

	float GetMouseSensibility() const { return MouseSensibility; }

	void MatchFinished(AActor* ViewTarget, int WinningTeam, float ViewBlendDuration = -1.f, float ResultWidgetDelay = 0.f);
	 
	class UGameplayWidget* GetGameplayWidget() const { return GameplayWidget; }

	UFUNCTION(Exec)
	void GrantInBaseTag();

	virtual void ReceivedPlayer() override;

	uint8 GetLocalLoadingProgress() const { return LocalLoadingProgress; }
	bool IsClientReadyReported() const { return bClientReadyReported; }

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION(Client, Reliable)
	void Client_MatchFinished(AActor* ViewTarget, int WinningTeam, float ViewBlendDuration, float ResultWidgetDelay);
	
	UPROPERTY(EditDefaultsOnly, Category = "View")
	float MatchFinishViewBlendTimeDuration = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "View")
	float MatchFinishViewBlendExp = 2.f;

	FTimerHandle ShowWinLoseStateTimerHandle;

	void SpawnGameplayWidget();

	UPROPERTY()
	ACPlayerCharacter* CPlayerCharacter;

	UPROPERTY(EditDefaultsOnly, Category = "Input", meta = (ClampMin = "0.1", ClampMax = "3.0", UIMin = "0.1", UIMAX = "3.0"))
	float MouseSensibility = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UGameplayWidget> GameplayWidgetclass;

	UPROPERTY()
	UGameplayWidget* GameplayWidget;

	UPROPERTY(Replicated)
	FGenericTeamId TeamID;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* UIInputMapping;
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* ShopToggleInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* ToggleGameplayMenuAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* FocusChatInputAction;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* ScoreboardInputAction;

	UPROPERTY(VisibleAnywhere, Category = "chat")
	class UChatComponent* ChatComponent;

	UFUNCTION()
	void ToggleShop();

	UFUNCTION()
	void ToggleGameplayMenu();

	UFUNCTION()
	void FocusChatInput();

	void ShowWinLoseState();

	UFUNCTION()
	void ShowScoreboard();

	UFUNCTION()
	void HideScoreboard();

	/*****************************/
	/********Match Loading********/
	/*****************************/
	void StartClientLoading();
	void HandleClientPreloadFinished();
	void UpdateLoadingProgress();
	void TryReportClientReady();

	bool bClientLoadingStarted = false;
	bool bClientAssetsLoaded = false;
	bool bClientPawnReady = false;
	bool bClientReadyReported = false;

	uint8 LocalLoadingProgress = 70;
	uint8 LastSentLoadingProgress = 0;

	FTimerHandle LoadingProgressTimerHandle;

	UFUNCTION(Server, Unreliable)
	void Server_ReportLoadingProgress(uint8 Progress);

	UFUNCTION(Server, Reliable)
	void Server_ReportClientReady();

	bool IsLoadingScreenVisible() const;
};