// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Inventory/Item/Data/ERNItemEnums.h"
#include "Shop/Data/ERNShopTypes.h"
#include "ERNUIFunctionLibrary.generated.h"

/**
 * 상점 및 UI에서 공통으로 사용할 블루프린트 헬퍼 함수 라이브러리
 */
UCLASS()
class PROJECTERN_API UERNUIFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 아이템 타입을 텍스트로 변환 (UI 표시용) */
	UFUNCTION(BlueprintPure, Category = "ERN|UI")
	static FText GetItemTypeText(EItemType ItemType);

	/** 상점 구매 결과값에 따른 토스트 알림 메시지와 색상 반환 */
	UFUNCTION(BlueprintPure, Category = "ERN|UI")
	static void GetPurchaseResultToastData(EERNTransactionResult Result, FText& OutMessage, FLinearColor& OutColor);

	/** 무기 종류를 텍스트로 변환 (UI 표시용) */
	UFUNCTION(BlueprintPure, Category = "ERN|UI")
	static FText GetWeaponTypeText(EWeaponType WeaponType);

	/** 소모품 종류를 텍스트로 변환 (UI 표시용) */
	UFUNCTION(BlueprintPure, Category = "ERN|UI")
	static FText GetConsumableTypeText(EConsumableType ConsumableType);
};
