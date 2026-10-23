// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/SplineWidget.h"

void USplineWidget::SetupSpline(const UUserWidget* InStartWidget, const UUserWidget* InEndWidget, const FVector2D& InStartPortLocalCoord, const FVector2D& InEndPortLocalCoord, const FVector2D& InStartPortDirection, const FVector2D& InEndPortDirection)
{
	StartWidget = InStartWidget;
	EndWidget = InEndWidget;
	StartPortLocalCoord = InStartPortLocalCoord;
	EndPortLocalCoord = InEndPortLocalCoord;
	StartPortDirection = InStartPortDirection;
	EndPortDirection = InEndPortDirection;
}

void USplineWidget::SetSplineStyle(const FLinearColor& InColor, float InThickness)
{
	Color = InColor;
	Thickness = InThickness;
}

int32 USplineWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	FVector2D StartPos = TestStartPos;
	FVector2D EndPos = TestEndPos;

	if (StartWidget && EndWidget)
	{
		StartPos = StartWidget->GetCachedGeometry().GetLocalPositionAtCoordinates(StartPortLocalCoord);
		EndPos = EndWidget->GetCachedGeometry().GetLocalPositionAtCoordinates(EndPortLocalCoord);
	}

	float MidY = (StartPos.Y + EndPos.Y) * 0.5f;

	TArray<FVector2D> LinePoints;
	LinePoints.Add(StartPos);
	LinePoints.Add(FVector2D(StartPos.X, MidY));
	LinePoints.Add(FVector2D(EndPos.X, MidY));
	LinePoints.Add(EndPos);

	FSlateDrawElement::MakeLines(
		OutDrawElements,
		++LayerId,
		AllottedGeometry.ToPaintGeometry(),
		LinePoints,
		ESlateDrawEffect::None,
		Color,
		true, // bAntialias
		Thickness
	);

	return LayerId;
}
