// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/ShopWidget.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Algo/Sort.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Components/VerticalBox.h"
#include "Framework/CAssetManager.h"
#include "GAS/CHeroAttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/ShopItemWidget.h"
#include "Widgets/ItemTreeWidget.h"
#include "Widgets/ShopCategoryButton.h"

namespace ShopSearch
{
	// 한글 호환 자모 초성 (ㄱ ㄲ ㄴ ㄷ ㄸ ㄹ ㅁ ㅂ ㅃ ㅅ ㅆ ㅇ ㅈ ㅉ ㅊ ㅋ ㅌ ㅍ ㅎ)
	static const TCHAR ChoseongTable[ 19 ] =
	{
		0x3131, 0x3132, 0x3134, 0x3137, 0x3138, 0x3139, 0x3141, 0x3142, 0x3143,
		0x3145, 0x3146, 0x3147, 0x3148, 0x3149, 0x314A, 0x314B, 0x314C, 0x314D, 0x314E
	};

	static bool IsHangulSyllable(TCHAR Char)
	{
		return Char >= 0xAC00 && Char <= 0xD7A3;
	}

	static bool IsChoseongJamo(TCHAR Char)
	{
		for ( TCHAR Choseong : ChoseongTable )
		{
			if ( Char == Choseong )
				return true;
		}
		return false;
	}

	static TCHAR GetChoseong(TCHAR Syllable)
	{
		return ChoseongTable[ (Syllable - 0xAC00) / 588 ];
	}

	// 공백 제거 + 소문자
	static FString Normalize(const FString& In)
	{
		FString Out;
		Out.Reserve(In.Len());

		for ( TCHAR Char : In )
		{
			if ( !FChar::IsWhitespace(Char) )
			{
				Out.AppendChar(FChar::ToLower(Char));
			}
		}

		return Out;
	}

	// 검색어 글자 1개가 이름 글자 1개와 맞는지 (검색어가 초성이면 이름 글자의 초성과 비교)
	static bool CharMatches(TCHAR QueryChar, TCHAR NameChar)
	{
		if ( QueryChar == NameChar )
			return true;

		return IsChoseongJamo(QueryChar) && IsHangulSyllable(NameChar) && GetChoseong(NameChar) == QueryChar;
	}

	// 정규화된 이름 안에 정규화된 검색어가 (초성 포함) 연속으로 들어있는지
	static bool Matches(const FString& NormalizedName, const FString& NormalizedQuery)
	{
		const int32 NameLen = NormalizedName.Len();
		const int32 QueryLen = NormalizedQuery.Len();

		if ( QueryLen == 0 )
			return true;

		for ( int32 Start = 0; Start + QueryLen <= NameLen; ++Start )
		{
			bool bAllMatched = true;
			for ( int32 Offset = 0; Offset < QueryLen; ++Offset )
			{
				if ( !CharMatches(NormalizedQuery[ Offset ], NormalizedName[ Start + Offset ]) )
				{
					bAllMatched = false;
					break;
				}
			}

			if ( bAllMatched )
				return true;
		}

		return false;
	}
}

void UShopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);

	LoadShopItems();

	ShopItemList->OnEntryWidgetGenerated().RemoveAll(this);
	ShopItemList->OnEntryWidgetGenerated().AddUObject(this, &UShopWidget::ShopWidgetGenerated);

	if ( APawn* OwnerPawn = GetOwningPlayerPawn() )
	{
		OwnerInventoryComponent = OwnerPawn->GetComponentByClass<UInventoryComponent>();
		OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerPawn);
	}
	
	if ( OwnerInventoryComponent )
	{
		UnbindInventoryEvents();
		OwnerInventoryComponent->OnPurchaseSucceeded.AddUObject(this, &UShopWidget::HandlePurchaseSucceeded);
		OwnerInventoryComponent->OnPurchaseFailed.AddUObject(this, &UShopWidget::HandlePurchaseFailed);
		OwnerInventoryComponent->OnSellFailed.AddUObject(this, &UShopWidget::HandleSellFailed);
		OwnerInventoryComponent->OnItemSold.AddUObject(this, &UShopWidget::HandleItemSold);

		OwnerInventoryComponent->OnItemAdded.AddUObject(this, &UShopWidget::HandleInventoryItemAdded);
		OwnerInventoryComponent->OnItemRemoved.AddUObject(this, &UShopWidget::HandleInventoryItemRemoved);
		OwnerInventoryComponent->OnItemStackCountChanged.AddUObject(this, &UShopWidget::HandleInventoryStackCountChanged);

		OwnerInventoryComponent->OnUndoAvailabilityChanged.AddUObject(this, &UShopWidget::HandleUndoAvailabilityChanged);
		OwnerInventoryComponent->OnUndoSucceeded.AddUObject(this, &UShopWidget::HandleUndoSucceeded);
		OwnerInventoryComponent->OnUndoFailed.AddUObject(this, &UShopWidget::HandleUndoFailed);
	}

	if ( OwnerASC )
	{
		OwnerASC->GetGameplayAttributeValueChangeDelegate(UCHeroAttributeSet::GetGoldAttribute()).AddUObject(this, &UShopWidget::HandleGoldChanged);
	}

	if ( PurchaseFailMessageText )
	{
		PurchaseFailMessageText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if ( SearchTextBox )
	{
		SearchTextBox->OnTextChanged.RemoveDynamic(this, &UShopWidget::HandleSearchTextChanged);
		SearchTextBox->OnTextChanged.AddDynamic(this, &UShopWidget::HandleSearchTextChanged);
		SearchTextBox->OnTextCommitted.RemoveDynamic(this, &UShopWidget::HandleSearchTextCommitted);
		SearchTextBox->OnTextCommitted.AddDynamic(this, &UShopWidget::HandleSearchTextCommitted);
	}

	if ( ClearSearchButton )
	{
		ClearSearchButton->OnClicked.RemoveDynamic(this, &UShopWidget::HandleClearSearchClicked);
		ClearSearchButton->OnClicked.AddDynamic(this, &UShopWidget::HandleClearSearchClicked);
		ClearSearchButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	if ( NoResultText )
	{
		NoResultText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if ( UndoButton )
	{
		UndoButton->OnClicked.RemoveDynamic(this, &UShopWidget::HandleUndoClicked);
		UndoButton->OnClicked.AddDynamic(this, &UShopWidget::HandleUndoClicked);
		UndoButton->SetIsEnabled(OwnerInventoryComponent && OwnerInventoryComponent->CanUndo());
	}
}

void UShopWidget::NativeDestruct()
{
	UnbindInventoryEvents();
	
	if ( OwnerASC )
	{
		OwnerASC->GetGameplayAttributeValueChangeDelegate(UCHeroAttributeSet::GetGoldAttribute()).RemoveAll(this);
	}

	if ( UWorld* World = GetWorld() )
	{
		World->GetTimerManager().ClearTimer(PurchaseFaileMessageTimerHandle);
	}

	Super::NativeDestruct();
}

void UShopWidget::LoadShopItems()
{
	UCAssetManager::Get().LoadShopItems(FStreamableDelegate::CreateUObject(this, &UShopWidget::ShopItemLoadFinished));
}

void UShopWidget::ShopItemLoadFinished()
{
	AllShopItems.Reset();
	UCAssetManager::Get().GetLoadedShopItems(AllShopItems);

	BuildCategoryButtons();
	RefreshShopItemList();
}

void UShopWidget::ShopWidgetGenerated(UUserWidget& NewWidget)
{
	UShopItemWidget* ItemWidget = Cast<UShopItemWidget>(&NewWidget);
	if ( !ItemWidget )
		return;

	if ( OwnerInventoryComponent )
	{
		ItemWidget->OnItemPurchaseIssued.RemoveAll(OwnerInventoryComponent);
		ItemWidget->OnItemPurchaseIssued.AddUObject(OwnerInventoryComponent, &UInventoryComponent::TryPurchase);
	}

	ItemWidget->OnShopItemClicked.RemoveAll(this);
	ItemWidget->OnShopItemClicked.AddUObject(this, &UShopWidget::ShowItemCombination);
	ItemsMap.Add(ItemWidget->GetShopItem(), ItemWidget);
}

void UShopWidget::ShowItemCombination(const UShopItemWidget* ItemWidget)
{
	if (CombinationTree)
	{
		CombinationTree->DrawFromNode(ItemWidget);
	}
}

void UShopWidget::HandlePurchaseFailed(EPurchaseResult Reason, const UPA_ShopItem* Item)
{
	FText Message;

	switch ( Reason )
	{
	case EPurchaseResult::NotInBase:
		Message = FText::FromString(TEXT("기지에서만 구매할 수 있습니다"));
		break;
	case EPurchaseResult::AlreadyOwnedUnique:
		Message = FText::FromString(TEXT("이미 보유 중인 고유 아이템입니다"));
		break;
	case EPurchaseResult::NotEnoughGold:
		Message = FText::FromString(TEXT("골드가 부족합니다"));
		break;
	case EPurchaseResult::InventoryFull:
		Message = FText::FromString(TEXT("인벤토리가 가득 찼습니다"));
		break;
	case EPurchaseResult::CategoryLimitReached:
		Message = FText::FromString(TEXT("같은 종류의 아이템은 하나만 보유할 수 있습니다"));
		break;
	default:
		Message = FText::FromString(TEXT("아이템을 구매할 수 없습니다"));
		break;
	}

	if ( FailureSound )
	{
		UGameplayStatics::PlaySound2D(this, FailureSound);
	}

	ShowMessage(Message);
}

void UShopWidget::ClearPurchaseFailMessage()
{
	if ( PurchaseFailMessageText )
	{
		PurchaseFailMessageText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UShopWidget::HandlePurchaseSucceeded(const UPA_ShopItem* Item)
{
	if ( PurchaseSuccessSound )
	{
		UGameplayStatics::PlaySound2D(this, PurchaseSuccessSound);
	}
}

void UShopWidget::HandleItemSold(const UPA_ShopItem* Item)
{
	if ( SellSound )
	{
		UGameplayStatics::PlaySound2D(this, SellSound);
	}
}

void UShopWidget::HandleSellFailed()
{
	if ( FailureSound )
	{
		UGameplayStatics::PlaySound2D(this, FailureSound);
	}

	ShowMessage(FText::FromString(TEXT("기지에서만 판매할 수 있습니다.")));
}

void UShopWidget::ShowMessage(const FText& Message)
{
	if ( !PurchaseFailMessageText )
		return;

	PurchaseFailMessageText->SetText(Message);
	PurchaseFailMessageText->SetVisibility(ESlateVisibility::Visible);

	GetWorld()->GetTimerManager().SetTimer(PurchaseFaileMessageTimerHandle, this, &UShopWidget::ClearPurchaseFailMessage, 2.f, false);
}

void UShopWidget::UnbindInventoryEvents()
{
	if ( !OwnerInventoryComponent )
		return;

	OwnerInventoryComponent->OnPurchaseSucceeded.RemoveAll(this);
	OwnerInventoryComponent->OnPurchaseFailed.RemoveAll(this);
	OwnerInventoryComponent->OnSellFailed.RemoveAll(this);
	OwnerInventoryComponent->OnItemSold.RemoveAll(this);

	OwnerInventoryComponent->OnItemAdded.RemoveAll(this);
	OwnerInventoryComponent->OnItemRemoved.RemoveAll(this);
	OwnerInventoryComponent->OnItemStackCountChanged.RemoveAll(this);

	OwnerInventoryComponent->OnUndoAvailabilityChanged.RemoveAll(this);
	OwnerInventoryComponent->OnUndoSucceeded.RemoveAll(this);
	OwnerInventoryComponent->OnUndoFailed.RemoveAll(this);
}

void UShopWidget::SetShopOpen(bool bOpen)
{
	bShopOpen = bOpen;

	if ( bShopOpen && bItemStateDirty )
	{
		RefreshAllItemStates();
	}
}

void UShopWidget::HandleGoldChanged(const FOnAttributeChangeData& Data)
{
	RequestItemStateRefresh();
}

void UShopWidget::HandleInventoryItemAdded(const UInventoryItem* Item)
{
	RequestItemStateRefresh();
}

void UShopWidget::HandleInventoryItemRemoved(const FInventoryItemHandle& Handle)
{
	RequestItemStateRefresh();
}

void UShopWidget::HandleInventoryStackCountChanged(const FInventoryItemHandle& Handle, int NewCount)
{
	RequestItemStateRefresh();
}

void UShopWidget::RequestItemStateRefresh()
{
	// 닫혀 있으면 표시만 해두고 열 때 갱신
	if ( !bShopOpen )
	{
		bItemStateDirty = true;
		return;
	}

	// 조합 구매처럼 한 프레임에 이벤트가 여러 번 와도 다음 틱에 한 번만 갱신
	if ( bRefreshQueued )
		return;

	bRefreshQueued = true;
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UShopWidget::RefreshAllItemStates);
}

void UShopWidget::RefreshAllItemStates()
{
	bRefreshQueued = false;
	bItemStateDirty = false;

	if ( ShopItemList )
	{
		for ( UUserWidget* EntryWidget : ShopItemList->GetDisplayedEntryWidgets() )
		{
			if ( UShopItemWidget* ItemWidget = Cast<UShopItemWidget>(EntryWidget) )
			{
				ItemWidget->RefreshState();
			}
		}
	}

	if ( CombinationTree )
	{
		CombinationTree->RefreshNodeStates();
	}
}

void UShopWidget::BuildCategoryButtons()
{
	if ( !CategoryButtonBox || !CategoryButtonClass )
		return;

	CategoryButtonBox->ClearChildren();
	CategoryButtons.Reset();

	for ( const FShopCategoryInfo& Info : Categories )
	{
		UShopCategoryButton* Button = CreateWidget<UShopCategoryButton>(GetOwningPlayer(), CategoryButtonClass);
		if ( !Button )
			continue;

		CategoryButtonBox->AddChildToVerticalBox(Button);
		Button->InitCategory(Info.CategoryTag, Info.DisplayName, Info.Icon);
		Button->OnCategoryBtnClicked.AddUObject(this, &UShopWidget::HandleCategoryClicked);

		CategoryButtons.Add(Button);
	}

	HandleCategoryClicked(FGameplayTag());
}

void UShopWidget::HandleCategoryClicked(const FGameplayTag& CategoryTag)
{
	CurrentCategory = CategoryTag;

	if ( IsSearching() )
	{
		ClearSearch();   
		return;
	}

	UpdateCategoryButtonSelection();
	RefreshShopItemList();
}

void UShopWidget::RefreshShopItemList()
{
	if ( !ShopItemList )
		return;

	TArray<const UPA_ShopItem*> FilteredItems;

	for ( const UPA_ShopItem* ShopItem : AllShopItems )
	{
		if ( !ShopItem )
			continue;

		if ( IsSearching() )
		{
			const FString NormalizedName = ShopSearch::Normalize(ShopItem->GetItemName().ToString());
			if ( ShopSearch::Matches(NormalizedName, CurrentSearchQuery) )
			{
				FilteredItems.Add(ShopItem);
			}
		}
		else if ( !CurrentCategory.IsValid() || ShopItem->GetItemCategories().HasTag(CurrentCategory) )
		{
			FilteredItems.Add(ShopItem);
		}
	}

	if ( NoResultText )
	{
		NoResultText->SetVisibility(IsSearching() && FilteredItems.Num() == 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	Algo::Sort(FilteredItems, [] (const UPA_ShopItem* A, const UPA_ShopItem* B)
		{
			if ( !FMath::IsNearlyEqual(A->GetPrice(), B->GetPrice()) )
				return A->GetPrice() < B->GetPrice();

			return A->GetItemName().CompareTo(B->GetItemName()) < 0;
		});

	ShopItemList->ClearListItems();
	ItemsMap.Empty();

	for ( const UPA_ShopItem* ShopItem : FilteredItems )
	{
		ShopItemList->AddItem(const_cast< UPA_ShopItem* >(ShopItem));
	}

	ShopItemList->ScrollToTop();
}

void UShopWidget::UpdateCategoryButtonSelection()
{
	for ( UShopCategoryButton* Button : CategoryButtons )
	{
		if ( Button )
		{
			Button->SetSelected(!IsSearching() && Button->GetCategoryTag() == CurrentCategory);
		}
	}
}

void UShopWidget::HandleSearchTextChanged(const FText& Text)
{
	const FString NewQuery = ShopSearch::Normalize(Text.ToString());

	if ( ClearSearchButton )
	{
		ClearSearchButton->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}

	if ( NewQuery == CurrentSearchQuery )
		return;

	CurrentSearchQuery = NewQuery;

	UpdateCategoryButtonSelection();
	RefreshShopItemList();
}

void UShopWidget::HandleSearchTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	// 검색창에서 ESC
	if ( CommitMethod == ETextCommit::OnCleared )
	{
		ClearSearch();
	}
}

void UShopWidget::HandleClearSearchClicked()
{
	ClearSearch();
}

void UShopWidget::ClearSearch()
{
	// 먼저 비워두면 SetText로 OnTextChanged가 와도 중복 갱신하지 않음
	CurrentSearchQuery.Reset();

	if ( SearchTextBox )
	{
		SearchTextBox->SetText(FText::GetEmpty());
	}

	if ( ClearSearchButton )
	{
		ClearSearchButton->SetVisibility(ESlateVisibility::Collapsed);
	}

	UpdateCategoryButtonSelection();
	RefreshShopItemList();
}

void UShopWidget::HandleUndoClicked()
{
	if ( OwnerInventoryComponent )
	{
		OwnerInventoryComponent->TryUndo();
	}
}

void UShopWidget::HandleUndoAvailabilityChanged(bool bCanUndo)
{
	if ( UndoButton )
	{
		UndoButton->SetIsEnabled(bCanUndo);
	}
}

void UShopWidget::HandleUndoSucceeded()
{
	if ( UndoSound )
	{
		UGameplayStatics::PlaySound2D(this, UndoSound);
	}
}

void UShopWidget::HandleUndoFailed()
{
	if ( FailureSound )
	{
		UGameplayStatics::PlaySound2D(this, FailureSound);
	}

	ShowMessage(FText::FromString(TEXT("되돌릴 수 없습니다")));
}