// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/Player/ERNPlayerStatusTable.h"

void FERNPlayerStatusTable::OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName)
{
	FTableRowBase::OnDataTableChanged(InDataTable, InRowName);
	
	int32 InLevel = 0;
	if (LexTryParseString(InLevel, *InRowName.ToString()))
	{
		Level = InLevel;
	}
}
