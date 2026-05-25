// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/ERNInteractableWidget.h"
#include "ERNMinimapWidget.generated.h"

class UERNMinimapIconDataAsset;
class UMinimapMarkerWidget;
class UCanvasPanel;
/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNMinimapWidget : public UERNInteractableWidget
{
	GENERATED_BODY()

public:
	void PlayOpenAnimation();
	
protected:
	// Collapsed 로 위젯 숨김
	virtual void NativeConstruct() override;
	
	virtual void NativeDestruct() override;
	
	// 마우스 입력은 가능
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	// WASD는 무시
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	
public:
	// 월드 맵 좌표를 미니맵 UI 좌표로 변환
	FVector2D WorldToMapPosition(const FVector& WorldLocation) const;
	
	// 마커 생성
	void RebuildStaticMarkers();
	
	void RefreshStaticMarkers();
	
public:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCanvasPanel> MarkerLayer;
	
	UPROPERTY(EditDefaultsOnly, Category="Minimap")
	TSubclassOf<UMinimapMarkerWidget> MarkerWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category="Minimap")
	TObjectPtr<UERNMinimapIconDataAsset> IconDataAsset;
	
	UPROPERTY(EditAnywhere, Category="Minimap|Map")
	FVector2D WorldMin = FVector2D(-50000.f, -50000.f);
	
	UPROPERTY(EditAnywhere, Category="Minimap|Map")
	FVector2D WorldMax = FVector2D(50000.f, 50000.f);
	
	UPROPERTY(EditAnywhere, Category="Minimap|Map")
	FVector2D MapSize = FVector2D(1024.f, 1024.f);
	
private:
	void HandleTargetsChanged();
	
	
private:
	// 열기 애니메이션
	UPROPERTY(meta=(BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> FadeIn;
	
	FDelegateHandle TargetsChangedHandle;
	
	// 더티 플래그. 값이 변할 때 처리하는 기법
	bool bStaticMarkersDirty = true;
	
};
