// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/ERNInteractableWidget.h"
#include "ERNMinimapWidget.generated.h"

class UMinimapPlayerMarkerWidget;
class UERNMinimapIconDataAsset;
class UMinimapMarkerWidget;
class UCanvasPanel;
class APawn;
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
	
	// 짧은 주기 플레이어 위치, 방향 최신화
	virtual void NativeTick(const FGeometry& InGeometry, float InDeltaTime) override;

public:
	// 월드 맵 좌표를 미니맵 UI 좌표로 변환
	FVector2D WorldToMapPosition(const FVector& WorldLocation) const;
	
	// 맵 좌표 -> 월드 좌표 (Pin 찍을 때 사용)
	FVector MapToWorldPosition(const FVector2D& MapPosition) const;
	
	
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
	
	// 실제 월드 최소 좌표, 최대 좌표. 월드에 따라 설정 필요. 월드에 따라 자동으로 넘겨주는 함수 없음
	UPROPERTY(EditAnywhere, Category="Minimap|Map")
	FVector2D WorldMin = FVector2D(-50000.f, -50000.f);
	
	UPROPERTY(EditAnywhere, Category="Minimap|Map")
	FVector2D WorldMax = FVector2D(50000.f, 50000.f);
	
	UPROPERTY(EditAnywhere, Category="Minimap|Map")
	FVector2D MapSize = FVector2D(1024.f, 1024.f);
	
	// 플레이어 실시간 위치 & 방향
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCanvasPanel> PlayerMarkerLayer;
	
	UPROPERTY(EditDefaultsOnly, Category="Minimap")
	TSubclassOf<UMinimapPlayerMarkerWidget> PlayerMarkerWidgetClass;
	
private:
	void HandleTargetsChanged();
	
	void RefreshPlayerMarkers();
	float WorldYawToMapAngle(float WorldYaw) const;

private:
	// 열기 애니메이션
	UPROPERTY(meta=(BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> FadeIn;
	
	FDelegateHandle TargetsChangedHandle;
	
	// 더티 플래그. 값이 변할 때 처리하는 기법. 주요 고정 목표에 사용
	bool bStaticMarkersDirty = true;
	
	UPROPERTY()
	TMap<APawn*, TObjectPtr<UMinimapPlayerMarkerWidget>> PlayerMarkerWidgets;
};
