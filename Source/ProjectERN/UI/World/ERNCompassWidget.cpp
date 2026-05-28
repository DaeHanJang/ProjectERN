// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/World/ERNCompassWidget.h"

#include "EngineUtils.h"
#include "UI/World/ERNCompassMarkerWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Character/Player/ERNPlayerState.h"
#include "World/ERNMinimapSubsystem.h"
#include "World/ERNMinimapTargetPoint.h"
#include "World/ERNMinimapPinPoint.h"
#include "World/Data/ERNMinimapIconDataAsset.h"
#include "World/NightRainZoneManager.h"

namespace
{
	// 좌표 관례: +X = 동(Yaw 0), +Y = 남(Yaw 90), -X = 서(Yaw 180), -Y = 북(Yaw -90)
	constexpr float CardinalYaw_N = -90.f;
	constexpr float CardinalYaw_E = 0.f;
	constexpr float CardinalYaw_S = 90.f;
	constexpr float CardinalYaw_W = 180.f;
}

void UERNCompassWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (IconDataAsset == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UERNCompassWidget IconDataAsset is null"));
	}
}

void UERNCompassWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APlayerController* PC = GetOwningPlayer();
	APawn* MyPawn = GetOwningPlayerPawn();
	if (PC == nullptr || MyPawn == nullptr)
	{
		return;
	}

	const float CameraYaw = PC->GetControlRotation().Yaw;
	const FVector MyLocation = MyPawn->GetActorLocation();

	UpdateCardinals(CameraYaw);
	UpdatePlayers(MyLocation, CameraYaw);
	UpdatePins(MyLocation, CameraYaw);
	UpdateZoneCenter(MyLocation, CameraYaw);
}

float UERNCompassWidget::WorldYawToTarget(const FVector& From, const FVector& To)
{
	const FVector Delta = To - From;
	return FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
}

bool UERNCompassWidget::ComputeBarX(float TargetWorldYaw, float CameraYaw, float& OutPixelXFromCenter) const
{
	const float Rel = FRotator::NormalizeAxis(TargetWorldYaw - CameraYaw);
	if (FMath::Abs(Rel) > HalfFovDegrees)
	{
		return false;
	}

	const float HalfWidth = MarkerLayer ? MarkerLayer->GetCachedGeometry().GetLocalSize().X * 0.5f : 0.f;
	OutPixelXFromCenter = (Rel / HalfFovDegrees) * HalfWidth;
	return true;
}

void UERNCompassWidget::PositionWidget(UWidget* Widget, float PixelXFromCenter, bool bVisible) const
{
	if (Widget == nullptr)
	{
		return;
	}

	if (bVisible == false)
	{
		Widget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	Widget->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Widget->Slot))
	{
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));

		const FVector2D BarSize = MarkerLayer ? MarkerLayer->GetCachedGeometry().GetLocalSize() : FVector2D::ZeroVector;
		const float AbsX = BarSize.X * 0.5f + PixelXFromCenter;
		const float CenterY = BarSize.Y * 0.5f;

		CanvasSlot->SetPosition(FVector2D(AbsX, CenterY));
	}
}

void UERNCompassWidget::PositionByWorldYaw(UWidget* Widget, float TargetWorldYaw, float CameraYaw) const
{
	float PixelX = 0.f;
	const bool bVisible = ComputeBarX(TargetWorldYaw, CameraYaw, PixelX);
	PositionWidget(Widget, PixelX, bVisible);
}

void UERNCompassWidget::UpdateCardinals(float CameraYaw)
{
	PositionByWorldYaw(Cardinal_N, CardinalYaw_N, CameraYaw);
	PositionByWorldYaw(Cardinal_E, CardinalYaw_E, CameraYaw);
	PositionByWorldYaw(Cardinal_S, CardinalYaw_S, CameraYaw);
	PositionByWorldYaw(Cardinal_W, CardinalYaw_W, CameraYaw);
}

UERNCompassMarkerWidget* UERNCompassWidget::CreateMarker(UTexture2D* IconTexture)
{
	if (MarkerWidgetClass == nullptr || MarkerLayer == nullptr)
	{
		return nullptr;
	}

	UERNCompassMarkerWidget* Marker = CreateWidget<UERNCompassMarkerWidget>(this, MarkerWidgetClass);
	if (Marker == nullptr)
	{
		return nullptr;
	}

	Marker->SetIcon(IconTexture);

	if (UCanvasPanelSlot* MarkerSlot = MarkerLayer->AddChildToCanvas(Marker))
	{
		MarkerSlot->SetAutoSize(true);
		MarkerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	}

	return Marker;
}

void UERNCompassWidget::UpdatePlayers(const FVector& MyLocation, float CameraYaw)
{
	if (MarkerLayer == nullptr || MarkerWidgetClass == nullptr || IconDataAsset == nullptr || GetWorld() == nullptr)
	{
		return;
	}

	AGameStateBase* GameState = GetWorld()->GetGameState();
	if (GameState == nullptr)
	{
		return;
	}

	APawn* OwnPawn = GetOwningPlayerPawn();

	TSet<APawn*> CurrentPawns;

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		AERNPlayerState* ERNPlayerState = Cast<AERNPlayerState>(PlayerState);
		if (ERNPlayerState == nullptr)
		{
			continue;
		}

		APawn* PlayerPawn = ERNPlayerState->GetPawn();
		if (IsValid(PlayerPawn) == false || PlayerPawn == OwnPawn)
		{
			// 자기 자신은 나침반에 표시하지 않음
			continue;
		}

		CurrentPawns.Add(PlayerPawn);

		UERNCompassMarkerWidget* Marker = nullptr;
		if (TObjectPtr<UERNCompassMarkerWidget>* Found = PlayerMarkers.Find(PlayerPawn))
		{
			Marker = Found->Get();
		}

		if (IsValid(Marker) == false)
		{
			UTexture2D* IconTexture = IconDataAsset->FindIconTexture(ERNPlayerState->GetMinimapPlayerMarkerIconType());
			Marker = CreateMarker(IconTexture);
			if (Marker == nullptr)
			{
				continue;
			}
			PlayerMarkers.Add(PlayerPawn, Marker);
		}
		else
		{
			// 아이콘 타입은 변할 수 있으므로 매번 최신화
			Marker->SetIcon(IconDataAsset->FindIconTexture(ERNPlayerState->GetMinimapPlayerMarkerIconType()));
		}

		const float TargetYaw = WorldYawToTarget(MyLocation, PlayerPawn->GetActorLocation());
		PositionByWorldYaw(Marker, TargetYaw, CameraYaw);
	}

	// 사라진 플레이어 마커 정리
	for (auto It = PlayerMarkers.CreateIterator(); It; ++It)
	{
		APawn* TrackedPawn = It.Key();
		UERNCompassMarkerWidget* TrackedMarker = It.Value();

		if (IsValid(TrackedPawn) == false || CurrentPawns.Contains(TrackedPawn) == false || IsValid(TrackedMarker) == false)
		{
			if (TrackedMarker)
			{
				TrackedMarker->RemoveFromParent();
			}
			It.RemoveCurrent();
		}
	}
}

void UERNCompassWidget::UpdatePins(const FVector& MyLocation, float CameraYaw)
{
	if (MarkerLayer == nullptr || MarkerWidgetClass == nullptr || IconDataAsset == nullptr || GetWorld() == nullptr)
	{
		return;
	}

	UERNMinimapSubsystem* Subsystem = GetWorld()->GetSubsystem<UERNMinimapSubsystem>();
	if (Subsystem == nullptr)
	{
		return;
	}

	TArray<AERNMinimapTargetPoint*> Targets;
	Subsystem->GetTargets(Targets);

	TSet<AActor*> CurrentPins;

	for (AERNMinimapTargetPoint* Target : Targets)
	{
		AERNMinimapPinPoint* Pin = Cast<AERNMinimapPinPoint>(Target);
		if (IsValid(Pin) == false)
		{
			continue;
		}

		CurrentPins.Add(Pin);

		UERNCompassMarkerWidget* Marker = nullptr;
		if (TObjectPtr<UERNCompassMarkerWidget>* Found = PinMarkers.Find(Pin))
		{
			Marker = Found->Get();
		}

		if (IsValid(Marker) == false)
		{
			UTexture2D* IconTexture = IconDataAsset->FindIconTexture(Pin->IconType);
			Marker = CreateMarker(IconTexture);
			if (Marker == nullptr)
			{
				continue;
			}
			PinMarkers.Add(Pin, Marker);
		}
		else
		{
			Marker->SetIcon(IconDataAsset->FindIconTexture(Pin->IconType));
		}

		const float TargetYaw = WorldYawToTarget(MyLocation, Pin->GetMinimapWorldLocation());
		PositionByWorldYaw(Marker, TargetYaw, CameraYaw);
	}

	// 사라진 핀 마커 정리
	for (auto It = PinMarkers.CreateIterator(); It; ++It)
	{
		AActor* TrackedPin = It.Key();
		UERNCompassMarkerWidget* TrackedMarker = It.Value();

		if (IsValid(TrackedPin) == false || CurrentPins.Contains(TrackedPin) == false || IsValid(TrackedMarker) == false)
		{
			if (TrackedMarker)
			{
				TrackedMarker->RemoveFromParent();
			}
			It.RemoveCurrent();
		}
	}
}

void UERNCompassWidget::UpdateZoneCenter(const FVector& MyLocation, float CameraYaw)
{
	if (MarkerLayer == nullptr || MarkerWidgetClass == nullptr || IconDataAsset == nullptr)
	{
		return;
	}

	ANightRainZoneManager* ZoneManager = FindZoneManager();
	const bool bZoneActive = ZoneManager && ZoneManager->GetZoneState().Revision > 0;

	if (bZoneActive == false)
	{
		if (ZoneMarker)
		{
			ZoneMarker->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	if (IsValid(ZoneMarker) == false)
	{
		UTexture2D* IconTexture = IconDataAsset->FindIconTexture(EERNMinimapIconType::NightRainZoneCenter);
		ZoneMarker = CreateMarker(IconTexture);
		if (ZoneMarker == nullptr)
		{
			return;
		}
	}

	// 조여드는 현재 중심(GetCurrentCenter)이 아니라, 수축 목적지인 안전지역 원의 중심을 표시
	const FVector ZoneCenter = ZoneManager->GetZoneState().TargetCenter;
	const float TargetYaw = WorldYawToTarget(MyLocation, ZoneCenter);
	PositionByWorldYaw(ZoneMarker, TargetYaw, CameraYaw);
}

ANightRainZoneManager* UERNCompassWidget::FindZoneManager()
{
	if (CachedZoneManager.IsValid())
	{
		return CachedZoneManager.Get();
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	for (TActorIterator<ANightRainZoneManager> It(World); It; ++It)
	{
		ANightRainZoneManager* ZoneManager = *It;
		if (IsValid(ZoneManager))
		{
			CachedZoneManager = ZoneManager;
			return ZoneManager;
		}
	}

	return nullptr;
}
