// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Widgets/TreeNodeInterface.h"
#include "ItemTreeWidget.generated.h"

class UShopItemWidget;

struct FItemTreeNodeRecord
{
	TWeakObjectPtr<UShopItemWidget> Widget;
	int32 ParentIndex = INDEX_NONE;
	int32 Depth = 0;
};


UCLASS()
class UItemTreeWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void DrawFromNode(const ITreeNodeInterface* NodeInterface);
	void RefreshNodeStates();

private:
	void DrawStream(
		bool bUpperStream,
		const ITreeNodeInterface* StartingNodeInterface,
		UUserWidget* StartingWidget,
		class UCanvasPanelSlot* StartingNodeWidget,
		int StartingNodeDepth,
		float& NextLeafXPosition,
		TArray<UCanvasPanelSlot*>& OutStreamSlots,
		int32 ParentRecordIndex
	);
	void ClearTree();
	void ClearUpperItemsRow();
	void PopulateUpperItemsRow(const ITreeNodeInterface* NodeInterface);

	UUserWidget* CreateWidgetForNode(const ITreeNodeInterface* Node, class UCanvasPanelSlot*& OutCanvasSlot);
	void CreateConnection(const UUserWidget* From, UUserWidget* To);


	UPROPERTY(meta=(BindWidget))
	class UCanvasPanel* RootPanel;

	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* UpperItemsRow;

	UPROPERTY(EditDefaultsOnly, Category = "Tree")
	TSubclassOf<class UShopItemIconWidget> UpperRowIconWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Tree")
	FVector2D UpperRowIconSize = FVector2D{ 40.f, 40.f };

	const UObject* CurrentCenterItem;

	UPROPERTY(EditDefaultsOnly, Category = "Tree")
	FVector2D NodeSize = FVector2D{ 60.f };

	UPROPERTY(EditDefaultsOnly, Category = "Tree")
	FVector2D NodeGap = FVector2D{ 16.f, 30.f };

	UPROPERTY(EditDefaultsOnly, Category = "Tree")
	FLinearColor ConnectionColor = FLinearColor{ 0.8f, 0.8f, 0.8f, 1.f };

	UPROPERTY(EditDefaultsOnly, Category = "Tree")
	float ConnectionThickness = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = "Tree")
	FVector2D SourcePortLocalPos = FVector2D{ 0.5f, 0.9f };

	UPROPERTY(EditDefaultsOnly, Category = "Tree")
	FVector2D DestinationPortLocalPos = FVector2D{ 0.5f, 0.1f };

	UPROPERTY(EditDefaultsOnly, Category = "Tree")
	FVector2D SourcePortDirection = FVector2D{ 0.f, 90.f };

	UPROPERTY(EditDefaultsOnly, Category = "Tree")
	FVector2D DestinationPortDirection = FVector2D{ 0.f, 90.f };

	TArray<FItemTreeNodeRecord> NodeRecords;
	int32 AddNodeRecord(UUserWidget* Widget, int32 ParentIndex, int32 Depth);
};
