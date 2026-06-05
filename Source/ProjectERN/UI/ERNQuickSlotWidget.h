// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/Data/ERNInventoryList.h"
#include "Inventory/Item/Data/ERNItemEnums.h"
#include "ERNQuickSlotWidget.generated.h"

enum class EERNUIType : uint8;
class UERNEquipmentComponent;
class UERNInventorySlotWidget;
class UTexture2D;

/**
 * 메인 화면 좌측 하단에 위치할 장비 및 소모품 표시용 퀵슬롯 위젯
 */
UCLASS()
class PROJECTERN_API UERNQuickSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UERNQuickSlotWidget(const FObjectInitializer& ObjectInitializer);

	// 현재 캐릭터 컴포넌트에 이벤트 바인딩
	void RefreshFromCurrentCharacter();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// UI 매니저의 상호작용 UI 상태 변경 시 숨김/표시 처리
	UFUNCTION()
	void HandleUIStateChanged(EERNUIType UIType);

	// 장착 컴포넌트 가져오기
	UERNEquipmentComponent* GetEquipmentComponent() const;
	
	// 장비 슬롯 갱신 이벤트 핸들러
	UFUNCTION()
	void UpdateEquipableSlot(const FInventoryItemEntry& Entry);
	
	// 소모품 슬롯 갱신 이벤트 핸들러
	UFUNCTION()
	void UpdateConsumableSlot(const FInventoryItemEntry& Entry);

	// 등급에 따른 색
	FColor ItemGradeByColor(EItemGrade Grade = EItemGrade::None);

	// 현재 캐릭터 컴포넌트에 이벤트 언바인딩
	void UnbindFromCurrentComponent();

private:
	// 장비 슬롯 위젯
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UERNInventorySlotWidget> EquipableSlotWidget;
	
	// 소모품 슬롯 위젯
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UERNInventorySlotWidget> ConsumableSlotWidget;
	
	// 인벤토리 슬롯 이미지 (빈 슬롯 이미지용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="QuickSlot", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UTexture2D> BasicSlotImage = nullptr;
	
	// 소모품 슬롯 이미지 (빈 슬롯 이미지용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="QuickSlot", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UTexture2D> BasicConsumableSlotImage = nullptr;

	// 현재 바인딩된 장착 컴포넌트
	UPROPERTY(Transient)
	TWeakObjectPtr<UERNEquipmentComponent> BoundEquipmentComponent;
};
