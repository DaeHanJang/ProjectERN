// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ERNCompassWidget.generated.h"

class UCanvasPanel;
class UERNCompassMarkerWidget;
class UERNMinimapIconDataAsset;
class ANightRainZoneManager;
class APawn;

/**
 * 화면 상단 나침반 바.
 * 카메라(컨트롤 회전) 방향 기준으로 방위(N/E/S/W), 다른 플레이어, 미니맵 핀, 자기장 중심을 표시.
 * - 좌표 관례: +X = 동(E, Yaw 0), -Y = 북(N, Yaw -90)
 * - 가시 범위 ±HalfFovDegrees (기본 90°, 총 180°), 범위 밖은 숨김
 * - 매 프레임 NativeTick으로 갱신 (카메라 회전에 부드럽게 반응)
 */
UCLASS()
class PROJECTERN_API UERNCompassWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// CanvasPanel 자식이라 슬롯으로 위치 조정
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> CardinalLayer;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> MarkerLayer;

	// 방위 문자 위젯 (BP에 배치, CardinalLayer 자식)
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Cardinal_N;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Cardinal_E;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Cardinal_S;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Cardinal_W;

	// 마커 위젯 클래스 / 아이콘 데이터에셋
	UPROPERTY(EditDefaultsOnly, Category = "Compass")
	TSubclassOf<UERNCompassMarkerWidget> MarkerWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Compass")
	TObjectPtr<UERNMinimapIconDataAsset> IconDataAsset;

	// 가시 반각 (총 180° → 90°)
	UPROPERTY(EditAnywhere, Category = "Compass", meta = (ClampMin = "1.0", ClampMax = "180.0"))
	float HalfFovDegrees = 90.f;

private:
	// === 핵심 로직 ===
	// 두 지점 사이의 월드 방위각 (UE Yaw 관례: atan2(dY, dX))
	static float WorldYawToTarget(const FVector& From, const FVector& To);

	// 카메라 기준 상대각 → 바 중앙 기준 픽셀 X. 가시 범위 밖이면 false
	bool ComputeBarX(float TargetWorldYaw, float CameraYaw, float& OutPixelXFromCenter) const;

	// 위젯을 바 위 (중앙 기준 PixelX) 위치에 배치 + 가시성 적용. 세로는 바 중앙 정렬
	void PositionWidget(UWidget* Widget, float PixelXFromCenter, bool bVisible) const;

	// 월드 Yaw 하나를 바에 배치 (방위 문자/단순 마커 공용)
	void PositionByWorldYaw(UWidget* Widget, float TargetWorldYaw, float CameraYaw) const;

	// === 갱신 단계 ===
	void UpdateCardinals(float CameraYaw);
	void UpdatePlayers(const FVector& MyLocation, float CameraYaw);
	void UpdatePins(const FVector& MyLocation, float CameraYaw);
	void UpdateZoneCenter(const FVector& MyLocation, float CameraYaw);

	// 마커 생성 헬퍼 (레이어에 추가 + 슬롯 정렬)
	UERNCompassMarkerWidget* CreateMarker(UTexture2D* IconTexture);

	ANightRainZoneManager* FindZoneManager();

private:
	// 캐시
	UPROPERTY()
	TMap<APawn*, TObjectPtr<UERNCompassMarkerWidget>> PlayerMarkers;

	UPROPERTY()
	TMap<AActor*, TObjectPtr<UERNCompassMarkerWidget>> PinMarkers;

	UPROPERTY()
	TObjectPtr<UERNCompassMarkerWidget> ZoneMarker;

	TWeakObjectPtr<ANightRainZoneManager> CachedZoneManager;
};
