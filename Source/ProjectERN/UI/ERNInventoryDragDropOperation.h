// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "ERNInventoryDragDropOperation.generated.h"

/**
 * 인벤토리 슬롯 드래그 앤 드롭 페이로드 - 출발 슬롯 인덱스를 운반
 */
UCLASS()
class PROJECTERN_API UERNInventoryDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	// 드래그를 시작한 슬롯 인덱스
	UPROPERTY()
	int32 SourceSlotIndex = -1;
};
