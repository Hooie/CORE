// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/ItemTreeWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"     
#include "Components/HorizontalBoxSlot.h" 
#include "Components/SizeBox.h"
#include "GameFramework/Pawn.h"
#include "Inventory/InventoryComponent.h"
#include "Widgets/ShopItemWidget.h"
#include "Widgets/SplineWidget.h"
#include "Widgets/ShopItemIconWidget.h"
#include "Inventory/PA_ShopItem.h"    

void UItemTreeWidget::DrawFromNode(const ITreeNodeInterface* NodeInterface)
{
	if ( !NodeInterface )
		return;

	if ( CurrentCenterItem == NodeInterface->GetItemObject() )
		return;

	ClearTree();
	ClearUpperItemsRow();
	CurrentCenterItem = NodeInterface->GetItemObject();

	PopulateUpperItemsRow(NodeInterface);

	UCanvasPanelSlot* CenterWidgetPanelSlot = nullptr;
	UUserWidget* CenterWidget = CreateWidgetForNode(NodeInterface, CenterWidgetPanelSlot);
	CenterWidgetPanelSlot->SetPosition(FVector2D::Zero());

	const int32 CenterRecordIndex = AddNodeRecord(CenterWidget, INDEX_NONE, 0);

	TArray<UCanvasPanelSlot*> AllTreeSlots;
	AllTreeSlots.Add(CenterWidgetPanelSlot);

	float MaxY = 0.f;

	if ( NodeInterface->GetOutputs().Num() > 0 )
	{
		float NextLeafXPos = 0.f;
		TArray<UCanvasPanelSlot*> LowerStreamSlots;
		DrawStream(false, NodeInterface, CenterWidget, CenterWidgetPanelSlot, 0, NextLeafXPos, LowerStreamSlots, CenterRecordIndex);

		float LowerStreamXMax = NextLeafXPos - NodeSize.X - NodeGap.X;
		float LowerMoveAmt = 0.f - LowerStreamXMax / 2.0f;

		for ( UCanvasPanelSlot* StreamSlot : LowerStreamSlots )
		{
			StreamSlot->SetPosition(StreamSlot->GetPosition() + FVector2D{ LowerMoveAmt, 0.f });
			MaxY = FMath::Max(MaxY, StreamSlot->GetPosition().Y);
			AllTreeSlots.Add(StreamSlot);
		}

		FVector2D CenterPos = CenterWidgetPanelSlot->GetPosition();
		CenterWidgetPanelSlot->SetPosition(FVector2D(0.f, CenterPos.Y));
	}

	float VerticalCenterOffset = -(0.f + MaxY) / 2.f;

	for ( UCanvasPanelSlot* PanelSlot : AllTreeSlots )
	{
		PanelSlot->SetPosition(PanelSlot->GetPosition() + FVector2D{ 0.f, VerticalCenterOffset });
	}

	RefreshNodeStates();
}

void UItemTreeWidget::DrawStream(bool bUpperStream, const ITreeNodeInterface* StartingNodeInterface, UUserWidget* StartingNodeWidget, UCanvasPanelSlot* StartingNodeSlot, int StartingNodeDepth, float& NextLeafXPosition, TArray<UCanvasPanelSlot*>& OutStreamSlots, int32 ParentRecordIndex)
{
	TArray<const ITreeNodeInterface*> NextTreeNodeInterfaces = bUpperStream ? StartingNodeInterface->GetInputs() : StartingNodeInterface->GetOutputs();
	float StartingNodeYPos = (NodeSize.Y + NodeGap.Y) * StartingNodeDepth * (bUpperStream ? -1 : 1);

	if ( NextTreeNodeInterfaces.Num() == 0 )
	{
		StartingNodeSlot->SetPosition(FVector2D{ NextLeafXPosition, StartingNodeYPos });
		NextLeafXPosition += NodeSize.X + NodeGap.X;

		return;
	}

	float NextNodeXPosSum = 0;

	for ( const ITreeNodeInterface* NextTreeNodeInterface : NextTreeNodeInterfaces )
	{
		UCanvasPanelSlot* NextWidgetSlot;
		UUserWidget* NextWidget = CreateWidgetForNode(NextTreeNodeInterface, NextWidgetSlot);

		const int32 NextRecordIndex = AddNodeRecord(NextWidget, ParentRecordIndex, StartingNodeDepth + 1);

		OutStreamSlots.Add(NextWidgetSlot);
		if ( bUpperStream )
		{
			CreateConnection(NextWidget, StartingNodeWidget);
		}
		else
		{
			CreateConnection(StartingNodeWidget, NextWidget);
		}

		DrawStream(bUpperStream, NextTreeNodeInterface, NextWidget, NextWidgetSlot, StartingNodeDepth + 1, NextLeafXPosition, OutStreamSlots, NextRecordIndex);
		NextNodeXPosSum += NextWidgetSlot->GetPosition().X;
	}

	float StartingNodeXPos = NextNodeXPosSum / NextTreeNodeInterfaces.Num();
	StartingNodeSlot->SetPosition(FVector2D{ StartingNodeXPos, StartingNodeYPos });
}

int32 UItemTreeWidget::AddNodeRecord(UUserWidget* Widget, int32 ParentIndex, int32 Depth)
{
	FItemTreeNodeRecord& Record = NodeRecords.AddDefaulted_GetRef();
	Record.Widget = Cast<UShopItemWidget>(Widget);
	Record.ParentIndex = ParentIndex;
	Record.Depth = Depth;

	return NodeRecords.Num() - 1;
}

void UItemTreeWidget::RefreshNodeStates()
{
	if ( NodeRecords.Num() == 0 )
		return;

	TMap<const UPA_ShopItem*, int32> AvailableCounts;

	if ( APawn* OwnerPawn = GetOwningPlayerPawn() )
	{
		if ( UInventoryComponent* InventoryComponent = OwnerPawn->FindComponentByClass<UInventoryComponent>() )
		{
			for ( const UInventoryItem* OwnedItem : InventoryComponent->GetAllItems() )
			{
				if ( OwnedItem && OwnedItem->GetShopItem() )
				{
					AvailableCounts.FindOrAdd(OwnedItem->GetShopItem())++;
				}
			}
		}
	}

	TArray<int32> ProcessOrder;
	ProcessOrder.Reserve(NodeRecords.Num());
	for ( int32 Index = 0; Index < NodeRecords.Num(); ++Index )
	{
		ProcessOrder.Add(Index);
	}

	ProcessOrder.StableSort([ this ] (int32 A, int32 B)
		{
			return NodeRecords[ A ].Depth < NodeRecords[ B ].Depth;
		});

	TArray<bool> bCovered;
	bCovered.Init(false, NodeRecords.Num());

	for ( int32 Index : ProcessOrder )
	{
		const FItemTreeNodeRecord& Record = NodeRecords[ Index ];
		UShopItemWidget* NodeWidget = Record.Widget.Get();
		if ( !NodeWidget )
			continue;

		// 중앙(완성품)은 상점과 동일한 표시
		if ( Record.Depth == 0 )
		{
			NodeWidget->RefreshState();
			continue;
		}

		// 상위 재료를 이미 보유 → 이 하위 재료는 필요 없음: 비활성화 색만
		if ( Record.ParentIndex != INDEX_NONE && bCovered[ Record.ParentIndex ] )
		{
			bCovered[ Index ] = true;
			NodeWidget->ApplyTreeOwnership(false, true);
			continue;
		}

		// 아직 배정 안 된 보유 아이템이 있으면 이 칸에 배정
		int32* RemainingCount = AvailableCounts.Find(NodeWidget->GetShopItem());
		if ( RemainingCount && *RemainingCount > 0 )
		{
			--(*RemainingCount);
			bCovered[ Index ] = true;
			NodeWidget->ApplyTreeOwnership(true, true);
		}
		else
		{
			NodeWidget->ApplyTreeOwnership(false, false);
		}
	}
}

void UItemTreeWidget::ClearTree()
{
	RootPanel->ClearChildren();
	NodeRecords.Reset();
}

void UItemTreeWidget::ClearUpperItemsRow()
{
	if ( UpperItemsRow )
	{
		UpperItemsRow->ClearChildren();
	}
}

void UItemTreeWidget::PopulateUpperItemsRow(const ITreeNodeInterface* NodeInterface)
{
	if ( !UpperItemsRow || !UpperRowIconWidgetClass )
		return;

	for ( const ITreeNodeInterface* UpperNode : NodeInterface->GetInputs() )
	{
		if ( !UpperNode )
			continue;

		const UPA_ShopItem* UpperShopItem = Cast<UPA_ShopItem>(UpperNode->GetItemObject());
		if ( !UpperShopItem )
			continue;

		UShopItemIconWidget* IconWidget = CreateWidget<UShopItemIconWidget>(GetOwningPlayer(), UpperRowIconWidgetClass);
		if ( !IconWidget )
			continue;

		IconWidget->InitWithShopItem(UpperShopItem);

		IconWidget->OnIconClicked.AddLambda([ this, UpperNode ] (const UPA_ShopItem*)
			{
				DrawFromNode(UpperNode);
			});

		USizeBox* SizeBoxWrapper = NewObject<USizeBox>(this);
		SizeBoxWrapper->SetWidthOverride(UpperRowIconSize.X);
		SizeBoxWrapper->SetHeightOverride(UpperRowIconSize.Y);
		SizeBoxWrapper->AddChild(IconWidget);

		if ( UHorizontalBoxSlot* NewSlot = UpperItemsRow->AddChildToHorizontalBox(SizeBoxWrapper) )
		{
			NewSlot->SetPadding(FMargin(4.f));
		}
	}
}

UUserWidget* UItemTreeWidget::CreateWidgetForNode(const ITreeNodeInterface* Node, UCanvasPanelSlot*& OutCanvasSlot)
{
	if (!Node)
		return nullptr;

	UUserWidget* NodeWidget = Node->GetWidget();
	OutCanvasSlot = RootPanel->AddChildToCanvas(NodeWidget);

	if (OutCanvasSlot)
	{
		OutCanvasSlot->SetSize(NodeSize);
		OutCanvasSlot->SetAnchors(FAnchors(0.5f));
		OutCanvasSlot->SetAlignment(FVector2D(0.5f));
		OutCanvasSlot->SetZOrder(1);
	}

	return NodeWidget;
}

void UItemTreeWidget::CreateConnection(const UUserWidget* From, UUserWidget* To)
{
	if (!From || !To)
		return;

	USplineWidget* Connection = CreateWidget<USplineWidget>(GetOwningPlayer());
	UCanvasPanelSlot* ConnectionPanelSlot = RootPanel->AddChildToCanvas(Connection);

	if (ConnectionPanelSlot)
	{
		ConnectionPanelSlot->SetAnchors(FAnchors{0.f});
		ConnectionPanelSlot->SetAlignment(FVector2D{0.f});
		ConnectionPanelSlot->SetPosition(FVector2D::Zero());
		ConnectionPanelSlot->SetZOrder(0);
	}
	
	Connection->SetupSpline(From, To, SourcePortLocalPos, DestinationPortLocalPos, SourcePortDirection, DestinationPortDirection);
	Connection->SetSplineStyle(ConnectionColor, ConnectionThickness);
}
