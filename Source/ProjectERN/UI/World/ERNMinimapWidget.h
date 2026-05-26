// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/ERNInteractableWidget.h"
#include "ERNMinimapWidget.generated.h"

class ANightRainZoneManager;
class UMinimapNightRainZoneWidget;
class UMinimapPlayerMarkerWidget;
class UERNMinimapIconDataAsset;
class UMinimapMarkerWidget;
class UCanvasPanel;
class APawn;
class UWidget;
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
	
	// WASD는 무시
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

public:
	// 월드 맵 좌표를 미니맵 UI 좌표로 변환
	FVector2D WorldToMapPosition(const FVector& WorldLocation) const;
	
	// 맵 좌표 -> 월드 좌표 (Pin 찍을 때 사용)
	FVector MapToWorldPosition(const FVector2D& MapPosition) const;
	
	
	// 마커 생성
	void RebuildStaticMarkers();
	
	void RefreshStaticMarkers();
			
	// PlayerController에서 호출. 플레이어 위치 마커는 타이머로 갱신
	void RefreshPlayerMarkers();
	float WorldYawToMapAngle(float WorldYaw) const;
	
	// 자기장 + 플레이어 위치 최신화 같이 일정 주기로 진행
	void StartDynamicMinimapRefresh();
	void StopDynamicMinimapRefresh();
	
	// 지도 조작 관련
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	void ApplyMapTransform();
	// 확대 줌인 줌아웃 처리
	void SetMapZoomAtScreenPosition(float NewZoom, const FVector2D& ScreenPosition);
	void AddMapPanOffsetByScreenDelta(const FVector2D& CurrentScreenPosition);
	// 맵 밖으로 나가지 못하게 드래그 범위 제한
	void ClampMapPanOffset();
	// 마커 아이콘 크기는 줌 배율에 역보정을 걸어서 일정 크기 유지
	float GetMarkerRenderScale() const;
	void ApplyMarkerRenderScale();
	
public:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidget> MapViewport;
	
	// 레이어 관리 상위 위젯. 지도 이동, 줌인 기능등에 사용
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCanvasPanel> MapContent;
	
	// ---레이어 모음---
	// 자기장 레이어
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCanvasPanel> NightRainZoneLayer;
	
	// 주요 고정 건물 마커 레이어
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCanvasPanel> MarkerLayer;
	
	// 플레이어 실시간 위치 & 방향 마커 레이어
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UCanvasPanel> PlayerMarkerLayer;
	
	
	// ---위젯 클래스 모음---
	UPROPERTY(EditDefaultsOnly, Category="Minimap")
	TSubclassOf<UMinimapNightRainZoneWidget> CurrentNightRainZoneWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category="Minimap")
	TSubclassOf<UMinimapNightRainZoneWidget> TargetNightRainZoneWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category="Minimap")
	TSubclassOf<UMinimapPlayerMarkerWidget> PlayerMarkerWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category="Minimap")
	TSubclassOf<UMinimapMarkerWidget> MarkerWidgetClass;
	
	// 아이콘 텍스처
	UPROPERTY(EditDefaultsOnly, Category="Minimap|Texture")
	TObjectPtr<UERNMinimapIconDataAsset> IconDataAsset;
	
	// 실제 월드 최소 좌표, 최대 좌표. 월드에 따라 설정 필요. 월드에 따라 자동으로 넘겨주는 함수 없음
	UPROPERTY(EditAnywhere, Category="Minimap|Map")
	FVector2D WorldMin = FVector2D(-50000.f, -50000.f);
	
	UPROPERTY(EditAnywhere, Category="Minimap|Map")
	FVector2D WorldMax = FVector2D(50000.f, 50000.f);
	
	UPROPERTY(EditAnywhere, Category="Minimap|Map")
	FVector2D MapSize = FVector2D(1024.f, 1024.f);
	
private:
	void HandleTargetsChanged();

	void RefreshDynamicMinimapElements();
	
	void RefreshNightRainZone();
	void HideNightRainZoneWidgets();
	
	// 위젯, 위젯 클래스, 중심점, 반지름, zorder
	void UpdateNightRainZoneCircle(TObjectPtr<UMinimapNightRainZoneWidget>& ZoneWidget, TSubclassOf<UMinimapNightRainZoneWidget> ZoneWidgetClass, const FVector& WorldCenter, float WorldRadius, int32 ZOrder);
	
	ANightRainZoneManager* FindNightRainZoneManager();
	FVector2D WorldRadiusToMapRadius(float WorldRadius) const;
private:
	// 열기 애니메이션
	UPROPERTY(meta=(BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> FadeIn;
	
	FDelegateHandle TargetsChangedHandle;
	
	// 더티 플래그. 값이 변할 때 처리하는 기법. 주요 고정 목표에 사용
	bool bStaticMarkersDirty = true;
	
	// 플레이어 위치 위젯
	UPROPERTY()
	TMap<APawn*, TObjectPtr<UMinimapPlayerMarkerWidget>> PlayerMarkerWidgets;
	
	// 틱 갱신 주기
	UPROPERTY(EditAnywhere, Category="Minimap|Dynamic", meta=(ClampMin="0.01", AllowPrivateAccess = "true"))
	float DynamicMinimapRefreshInterval  = 0.2f;
	
	FTimerHandle DynamicMinimapRefreshTimerHandle;
	
	// 자기장
	TWeakObjectPtr<ANightRainZoneManager> CachedNightRainZoneManager;
	
	UPROPERTY()
	TObjectPtr<UMinimapNightRainZoneWidget> CurrentZoneWidget;

	UPROPERTY()
	TObjectPtr<UMinimapNightRainZoneWidget> TargetZoneWidget;
	
	
	// 지도 조작 관련
	UPROPERTY(EditAnywhere, Category="Minimap|Transform", meta=(ClampMin="0.1", AllowPrivateAccess = "true"))
	float MinMapZoom = 1.0f;
	
	UPROPERTY(EditAnywhere, Category="Minimap|Transform", meta=(ClampMin="0.01", AllowPrivateAccess = "true"))
	float MaxMapZoom = 4.0f;
	
	UPROPERTY(EditAnywhere, Category="Minimap|Transform", meta=(ClampMin="1.01", AllowPrivateAccess = "true"))
	float MouseWheelZoomMultiplier = 1.15f;
	
	float CurrentMapZoom = 1.0f;
	FVector2D CurrentMapPanOffset = FVector2D::ZeroVector;
	
	bool bIsDraggingMap = false;
	FVector2D LastDragScreenPosition = FVector2D::ZeroVector;
};
