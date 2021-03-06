
#include "DataTableRowHandleCustom.h"
#include "DataTableEditorUtils.h"
#include "Editor.h"
#include "IDetailChildrenBuilder.h"
//#include "DataTableEditor/Public/DataTableRowUtlis.h"
#include "Editor/UnrealEd/Public/DataTableEditorUtils.h"

#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Input/SSearchBox.h"

#define LOCTEXT_NAMESPACE "FDataTableRowHandleCustom"

TSharedPtr<FName> FDataTableRowHandleCustom::InitWidgetContent()
{
	TSharedPtr<FName> InitialValue = MakeShared<FName>();

	FName RowName;
	const FPropertyAccess::Result RowResult = RowNamePropertyHandle->GetValue(RowName);
	RowNames.Empty();

	/** Get the properties we wish to work with */
	const UDataTable* DataTable = nullptr;
	DataTablePropertyHandle->GetValue((UObject*&)DataTable);

	if (DataTable != nullptr)
	{
		/** Extract all the row names from the RowMap */
		for (TMap<FName, uint8*>::TConstIterator Iterator(DataTable->GetRowMap()); Iterator; ++Iterator)
		{
			/** Create a simple array of the row names */
			TSharedRef<FName> RowNameItem = MakeShared<FName>(Iterator.Key());
			RowNames.Add(RowNameItem);

			/** Set the initial value to the currently selected item */
			if (Iterator.Key() == RowName)
			{
				InitialValue = RowNameItem;
			}
		}
	}

	/** Reset the initial value to ensure a valid entry is set */
	if (RowResult != FPropertyAccess::MultipleValues)
	{
		RowNamePropertyHandle->SetValue(*InitialValue);
	}

	return InitialValue;
}

void FDataTableRowHandleCustom::CustomizeHeader(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	this->StructPropertyHandle = InStructPropertyHandle;

	if (StructPropertyHandle->HasMetaData(TEXT("RowType")))
	{
		const FString& RowType = StructPropertyHandle->GetMetaData(TEXT("RowType"));
		RowTypeFilter = FName(*RowType);
	}

	FSimpleDelegate OnDataTableChangedDelegate = FSimpleDelegate::CreateSP(this, &FDataTableRowHandleCustom::OnDataTableChanged);
	StructPropertyHandle->SetOnPropertyValueChanged(OnDataTableChangedDelegate);
	
	HeaderRow
	.NameContent()
	[
		InStructPropertyHandle->CreatePropertyNameWidget(FText::GetEmpty(), FText::GetEmpty(), false)
	];

	FDataTableEditorUtils::AddSearchForReferencesContextMenu(HeaderRow, FExecuteAction::CreateSP(this, &FDataTableRowHandleCustom::OnSearchForReferences));
}

void FDataTableRowHandleCustom::CustomizeChildren(TSharedRef<class IPropertyHandle> InStructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	/** Get all the existing property handles */
	DataTablePropertyHandle = InStructPropertyHandle->GetChildHandle("DataTable");
	RowNamePropertyHandle = InStructPropertyHandle->GetChildHandle("RowName");

	if (DataTablePropertyHandle->IsValidHandle() && RowNamePropertyHandle->IsValidHandle())
	{
		/** Queue up a refresh of the selected item, not safe to do from here */
		StructCustomizationUtils.GetPropertyUtilities()->EnqueueDeferredAction(FSimpleDelegate::CreateSP(this, &FDataTableRowHandleCustom::OnDataTableChanged));

		/** Setup Change callback */
		FSimpleDelegate OnDataTableChangedDelegate = FSimpleDelegate::CreateSP(this, &FDataTableRowHandleCustom::OnDataTableChanged);
		DataTablePropertyHandle->SetOnPropertyValueChanged(OnDataTableChangedDelegate);

		/** Construct a asset picker widget with a custom filter */
		StructBuilder.AddCustomRow(LOCTEXT("DataTable_TableName", "Data Table"))
			.NameContent()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DataTable_TableName", "Data Table"))
				.Font(StructCustomizationUtils.GetRegularFont())
			]
			.ValueContent()
			.MaxDesiredWidth(0.0f) // don't constrain the combo button width
			[
				SNew(SObjectPropertyEntryBox)
				.PropertyHandle(DataTablePropertyHandle)
				.AllowedClass(UDataTable::StaticClass())
				.OnShouldFilterAsset(this, &FDataTableRowHandleCustom::ShouldFilterAsset)
			];

		/** Construct a combo box widget to select from a list of valid options */
		StructBuilder.AddCustomRow(LOCTEXT("DataTable_RowName", "Row Name"))
			.NameContent()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DataTable_RowName", "Row Name"))
				.Font(StructCustomizationUtils.GetRegularFont())
			]
			.ValueContent()
			.MaxDesiredWidth(0.0f) // don't constrain the combo button width
			[
				SAssignNew(RowNameComboButton, SComboButton)
				.ToolTipText(this, &FDataTableRowHandleCustom::GetRowNameComboBoxContentText)
				.OnGetMenuContent(this, &FDataTableRowHandleCustom::GetListContent)
				.OnComboBoxOpened(this, &FDataTableRowHandleCustom::HandleMenuOpen)
				.ContentPadding(FMargin(2.0f, 2.0f))
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(this, &FDataTableRowHandleCustom::GetRowNameComboBoxContentText)
				]
			];
	}
}

void FDataTableRowHandleCustom::HandleMenuOpen()
{
	FSlateApplication::Get().SetKeyboardFocus(SearchBox);
}

void FDataTableRowHandleCustom::OnSearchForReferences()
{
	if (CurrentSelectedItem.IsValid() && !CurrentSelectedItem->IsNone() && DataTablePropertyHandle.IsValid() && DataTablePropertyHandle->IsValidHandle())
	{
		UObject* SourceDataTable;
		DataTablePropertyHandle->GetValue(SourceDataTable);
		
		TArray<FAssetIdentifier> AssetIdentifiers;
		AssetIdentifiers.Add(FAssetIdentifier(SourceDataTable, *CurrentSelectedItem));

		FEditorDelegates::OnOpenReferenceViewer.Broadcast(AssetIdentifiers, FReferenceViewerParams());
	}
}

TSharedRef<SWidget> FDataTableRowHandleCustom::GetListContent()
{
	SAssignNew(RowNameComboListView, SListView<TSharedPtr<FName>>)
		.ListItemsSource(&RowNames)
		.ListItemsSource(&RowNames)
		.OnSelectionChanged(this, &FDataTableRowHandleCustom::OnSelectionChanged)
		.OnGenerateRow(this, &FDataTableRowHandleCustom::HandleRowNameComboBoxGenarateWidget)
		.SelectionMode(ESelectionMode::Single);

	// Ensure no filter is applied at the time the menu opens
	OnFilterTextChanged(FText::GetEmpty());

	if (CurrentSelectedItem.IsValid())
	{
		RowNameComboListView->SetSelection(CurrentSelectedItem);
	}

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SAssignNew(SearchBox, SSearchBox)
			.OnTextChanged(this, &FDataTableRowHandleCustom::OnFilterTextChanged)
		]
		
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			SNew(SBox)
			.MaxDesiredHeight(600)
			[
				RowNameComboListView.ToSharedRef()
			]
		];
}

void FDataTableRowHandleCustom::OnDataTableChanged()
{
	CurrentSelectedItem = InitWidgetContent();
	if (RowNameComboListView.IsValid())
	{
		RowNameComboListView->SetSelection(CurrentSelectedItem);
		RowNameComboListView->RequestListRefresh();
	}
}

TSharedRef<ITableRow> FDataTableRowHandleCustom::HandleRowNameComboBoxGenarateWidget(TSharedPtr<FName> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return
		SNew(STableRow<TSharedPtr<FName>>, OwnerTable)
		[
			SNew(STextBlock).Text(FText::FromName(*InItem))
		];
}

FText FDataTableRowHandleCustom::GetRowNameComboBoxContentText() const
{
	FName RowNameValue;
	const FPropertyAccess::Result RowResult = RowNamePropertyHandle->GetValue(RowNameValue);
	if (RowResult == FPropertyAccess::Success)
	{
		if (RowNameValue.IsNone())
		{
			return LOCTEXT("DataTable_None", "None");
		}
		return FText::FromName(RowNameValue);
	}
	else if (RowResult == FPropertyAccess::Fail)
	{
		return LOCTEXT("DataTable_None", "None");
	}
	else
	{
		return LOCTEXT("MultipleValues", "Multiple Values");
	}
}

void FDataTableRowHandleCustom::OnSelectionChanged(TSharedPtr<FName> SelectedItem, ESelectInfo::Type SelectInfo)
{
	if (SelectedItem.IsValid())
	{
		CurrentSelectedItem = SelectedItem;
		RowNamePropertyHandle->SetValue(*SelectedItem);

		// Close the combo
		RowNameComboButton->SetIsOpen(false);
	}
}

void FDataTableRowHandleCustom::OnFilterTextChanged(const FText& InFilterText)
{
	FString CurrentFilterText = InFilterText.ToString();

	RowNames.Empty();

	/** Get the properties we wish to work with */
	const UDataTable* DataTable = nullptr;
	DataTablePropertyHandle->GetValue((UObject*&)DataTable);

	TArray<FName> AllRowNames;
	if (DataTable != nullptr)
	{
		for (TMap<FName, uint8*>::TConstIterator Iterator(DataTable->GetRowMap()); Iterator; ++Iterator)
		{
			AllRowNames.Add(Iterator.Key());
		}

		// Sort the names alphabetically.
		AllRowNames.Sort(FNameLexicalLess());
	}

	for (const FName& RowName : AllRowNames)
	{
		if (CurrentFilterText.IsEmpty() || RowName.ToString().Contains(CurrentFilterText))
		{
			TSharedRef<FName> RowNameItem = MakeShared<FName>(RowName);
			RowNames.Add(RowNameItem);
		}
	}

	RowNameComboListView->RequestListRefresh();
}

bool FDataTableRowHandleCustom::ShouldFilterAsset(const struct FAssetData& AssetData)
{
	if (!RowTypeFilter.IsNone())
	{
		const UDataTable* DataTable = Cast<UDataTable>(AssetData.GetAsset());
		if (DataTable->RowStruct && DataTable->RowStruct->GetFName() == RowTypeFilter)
		{
			return false;
		}
		return true;
	}
	return false;
}
#undef LOCTEXT_NAMESPACE
