// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/World/ERNMinimapWidget.h"

#include "EngineUtils.h"
#include "MinimapMarkerWidget.h"
#include "MinimapNightRainZoneWidget.h"
#include "MinimapPlayerMarkerWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "World/ERNMinimapSubsystem.h"
#include "World/ERNMinimapTargetPoint.h"
#include "World/Data/ERNMinimapIconDataAsset.h"
#include "Character/Player/ERNPlayerController.h"
#include "Character/Player/ERNPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "World/NightRainZoneManager.h"


void UERNMinimapWidget::PlayOpenAnimation()
{
	if (FadeIn)
	{
		PlayAnimation(FadeIn);
	}
	else
	{
		UE_LOG(LogTemp,Warning,TEXT("MinimapWidget FadeIn is null"));
	}
}

void UERNMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	SetVisibility(ESlateVisibility::Collapsed);
	
	if (UERNMinimapSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UERNMinimapSubsystem>() : nullptr)
	{
		TargetsChangedHandle = Subsystem->OnTargetsChanged.AddUObject(
			this,
			&UERNMinimapWidget::HandleTargetsChanged);
	}

	bStaticMarkersDirty = true;
}

void UERNMinimapWidget::NativeDestruct()
{
	StopDynamicMinimapRefresh();
	
	if (TargetsChangedHandle.IsValid())
	{
		if (UERNMinimapSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UERNMinimapSubsystem>() : nullptr)
		{
			Subsystem->OnTargetsChanged.Remove(TargetsChangedHandle);
			TargetsChangedHandle.Reset();
		}
	}
	
	Super::NativeDestruct();
}

FReply UERNMinimapWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::W ||
		Key == EKeys::A ||
		Key == EKeys::S ||
		Key == EKeys::D)
	{
		return FReply::Unhandled();
	}

	// 콘솔 키 처리
	// 줌인
	if (Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		const FVector2D ZoomCenter = GetMapViewportCenterPosition();
		const float ZoomMultiplier = FMath::Pow(MouseWheelZoomMultiplier, 1.f);
		SetMapZoomAtScreenPosition(CurrentMapZoom * ZoomMultiplier, ZoomCenter);
		return FReply::Handled();
	}
	// 줌아웃
	if (Key == EKeys::Gamepad_FaceButton_Right)
	{
		const FVector2D ZoomCenter = GetMapViewportCenterPosition();
		const float ZoomMultiplier = FMath::Pow(MouseWheelZoomMultiplier, -1.f);
		SetMapZoomAtScreenPosition(CurrentMapZoom * ZoomMultiplier, ZoomCenter);
		return FReply::Handled();
	}
	// 핀
	if (Key == EKeys::Gamepad_FaceButton_Left)
	{
		const FVector2D PinCenter = GetMapViewportCenterPosition();
		const FVector2D MapPosition = MapContent->GetCachedGeometry().AbsoluteToLocal(PinCenter);
		const FVector WorldLocation = MapToWorldPosition(MapPosition);
		
		if (AERNPlayerController* PC = Cast<AERNPlayerController>(GetOwningPlayer()))
		{
			PC->Server_RequestCreateMinimapPin(WorldLocation);
		}
		
		return FReply::Handled();
	}
	
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UERNMinimapWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry,
	const FAnalogInputEvent& InAnalogEvent)
{
	const FKey Key = InAnalogEvent.GetKey();
	const float RawValue = InAnalogEvent.GetAnalogValue();
	float Value;
	
	if (FMath::Abs(RawValue) < 0.2f)
	{
		Value = 0.f;
	}
	else
	{
		Value = RawValue;
	}
	
	// IA를 통한 값 전달이 아닌 위젯에서의 값 전달
	if (Key == EKeys::Gamepad_RightX)
	{
		GamepadPanInput.X = Value * -1;
		return FReply::Handled();
	}
	
	if (Key == EKeys::Gamepad_RightY)
	{
		GamepadPanInput.Y = Value;
		return FReply::Handled();
	}
	
	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogEvent);
}

FVector2D UERNMinimapWidget::WorldToMapPosition(const FVector& WorldLocation) const
{
	const FVector2D WorldSize = WorldMax - WorldMin;
	
	if (FMath::IsNearlyZero(WorldSize.X) || FMath::IsNearlyZero(WorldSize.Y))
	{
		return FVector2D::ZeroVector;
	}
	
	const float NormalizedX = FMath::Clamp((WorldLocation.X - WorldMin.X) / WorldSize.X, 0.f, 1.f);
	const float NormalizedY = FMath::Clamp((WorldLocation.Y - WorldMin.Y) / WorldSize.Y, 0.f, 1.f);
	
	return FVector2D(NormalizedX * MapSize.X, NormalizedY * MapSize.Y);
}

FVector UERNMinimapWidget::MapToWorldPosition(const FVector2D& MapPosition) const
{
	const FVector2D WorldSize = WorldMax - WorldMin;
	
	if (FMath::IsNearlyZero(MapSize.X) || FMath::IsNearlyZero(MapSize.Y))
	{
		return FVector::ZeroVector;
	}
	
	const float NormalizedX = FMath::Clamp(MapPosition.X / MapSize.X, 0.f, 1.f);
	const float NormalizedY = FMath::Clamp(MapPosition.Y / MapSize.Y, 0.f, 1.f);
	
	const float WorldX = WorldMin.X + NormalizedX * WorldSize.X;
	const float WorldY = WorldMin.Y + NormalizedY * WorldSize.Y;
	
	return FVector(WorldX, WorldY, 0.f);
}

void UERNMinimapWidget::RebuildStaticMarkers()
{
	if (MarkerLayer == nullptr || MarkerWidgetClass ==  nullptr)
	{
		return;
	}
	
	if (IconDataAsset == nullptr)
	{
		UE_LOG(LogTemp,Warning,TEXT("UERNMinimapWidget IconDataAsset is null"));
		return;
	}
	
	MarkerLayer->ClearChildren();
	
	UERNMinimapSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UERNMinimapSubsystem>() : nullptr;
	
	if (Subsystem == nullptr)
	{
		return;
	}
	TArray<AERNMinimapTargetPoint*> Targets;
	Subsystem->GetTargets(Targets);
	
	for (AERNMinimapTargetPoint* Target : Targets)
	{
		if (IsValid(Target) == false || Target->bVisibleOnMinimap == false)
		{
			continue;
		}
		
		UTexture2D* IconTexture = IconDataAsset->FindIconTexture(Target->IconType);
		if (IconTexture == nullptr)
		{
			continue;
		}
		
		UMinimapMarkerWidget* Marker = CreateWidget<UMinimapMarkerWidget>(this, MarkerWidgetClass);
		if (Marker == nullptr)
		{
			continue;
		}
		
		Marker->SetMarkerData(IconTexture, Target);
		
		if (UCanvasPanelSlot* MarkerSlot = MarkerLayer->AddChildToCanvas(Marker))
		{
			MarkerSlot->SetAutoSize(true);
			MarkerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			MarkerSlot->SetPosition(WorldToMapPosition(Target->GetMinimapWorldLocation()));
		}
	}
	
	bStaticMarkersDirty = false;
	
	ApplyMarkerRenderScale();
}

void UERNMinimapWidget::RefreshStaticMarkers()
{
	if (bStaticMarkersDirty)
	{
		RebuildStaticMarkers();
	}
}

void UERNMinimapWidget::HandleTargetsChanged()
{
	bStaticMarkersDirty = true;
	
	// 보이는 대상만 최신화
	if (GetVisibility() == ESlateVisibility::Visible)
	{
		RebuildStaticMarkers();
	}
}

void UERNMinimapWidget::RefreshDynamicMinimapElements()
{
	RefreshPlayerMarkers();
	RefreshNightRainZone();
	
	ApplyMarkerRenderScale();
}

void UERNMinimapWidget::HideNightRainZoneWidgets()
{
	if (CurrentZoneWidget)
	{
		CurrentZoneWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
	
	if (TargetZoneWidget)
	{
		TargetZoneWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UERNMinimapWidget::UpdateNightRainZoneCircle(TObjectPtr<UMinimapNightRainZoneWidget>& ZoneWidget, TSubclassOf<UMinimapNightRainZoneWidget> ZoneWidgetClass,
													const FVector& WorldCenter, float WorldRadius, int32 ZOrder)
{
	if (NightRainZoneLayer == nullptr || ZoneWidgetClass == nullptr)
	{
		return;
	}
	
	if (IsValid(ZoneWidget.Get()) == false)
	{
		ZoneWidget = CreateWidget<UMinimapNightRainZoneWidget>(this, ZoneWidgetClass);
		if (ZoneWidget == nullptr)
		{
			return;
		}
		
		ZoneWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		
		if (UCanvasPanelSlot* ZoneSlot = NightRainZoneLayer->AddChildToCanvas(ZoneWidget))
		{
			ZoneSlot->SetAutoSize(false);
			ZoneSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			ZoneSlot->SetZOrder(ZOrder);
		}
	}
	
	ZoneWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	
	const FVector2D MapCenter = WorldToMapPosition(WorldCenter);
	const FVector2D MapRadius = WorldRadiusToMapRadius(WorldRadius);
	
	ZoneWidget->UpdateZone(MapCenter, MapRadius);
}

void UERNMinimapWidget::RefreshNightRainZone()
{
	if (NightRainZoneLayer == nullptr || CurrentNightRainZoneWidgetClass == nullptr)
	{		
		HideNightRainZoneWidgets();
		return;
	}
	
	ANightRainZoneManager* ZoneManager = FindNightRainZoneManager();
	if (ZoneManager == nullptr)
	{
		//자기장을 안쓰는 맵에서는 자기장 숨김
		HideNightRainZoneWidgets();
		return;
	}
	
	const FNightRainZoneState& ZoneState = ZoneManager->GetZoneState();

	// 아직 신호가 오지 않은 경우 숨김
	if (ZoneState.Revision <= 0)
	{
		HideNightRainZoneWidgets();
		return;
	}
	
	const FVector CurrentWorldCenter = ZoneManager->GetCurrentCenter();
	const float CurrentWorldRadius = ZoneManager->GetCurrentRadius();
	
	UpdateNightRainZoneCircle(CurrentZoneWidget, CurrentNightRainZoneWidgetClass, CurrentWorldCenter, CurrentWorldRadius, 1);
	
	// 수축중일 때에는 목표 자기장도 같이 보이도록 설정
	if (ZoneState.bShrinking && TargetNightRainZoneWidgetClass)
	{
		UpdateNightRainZoneCircle(TargetZoneWidget, TargetNightRainZoneWidgetClass, ZoneState.TargetCenter, ZoneState.TargetRadius, 2);
	}
	else if (TargetZoneWidget)
	{
		TargetZoneWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

ANightRainZoneManager* UERNMinimapWidget::FindNightRainZoneManager()
{
	// 이미 초기화 된 상태
	if (CachedNightRainZoneManager.IsValid())
	{
		return CachedNightRainZoneManager.Get();
	}
	
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}
	
	for (TActorIterator<ANightRainZoneManager> ZoneManagerIterator(World); ZoneManagerIterator; ++ZoneManagerIterator)
	{
		ANightRainZoneManager* ZoneManager = *ZoneManagerIterator;
		if (IsValid(ZoneManager))
		{
			CachedNightRainZoneManager = ZoneManager;
			return ZoneManager;
		}
	}
	
	return nullptr;
}

FVector2D UERNMinimapWidget::WorldRadiusToMapRadius(float WorldRadius) const
{
	const FVector2D WorldSize = WorldMax - WorldMin;
	
	if (FMath::IsNearlyZero(WorldSize.X) || FMath::IsNearlyZero(WorldSize.Y))
	{
		return FVector2D::ZeroVector;
	}
	
	const float MapRadiusX = WorldRadius / WorldSize.X * MapSize.X;
	const float MapRadiusY = WorldRadius / WorldSize.Y * MapSize.Y;
	
	return FVector2D(MapRadiusX, MapRadiusY);
}

FVector2D UERNMinimapWidget::GetMapViewportCenterPosition() const
{
	if (MapViewport == nullptr)
	{
		return FVector2D::ZeroVector;
	}
	
	// 저장된 지형의 중심을 미니맵 위젯에 맞게 변환
	const FGeometry& ViewportGeometry = MapViewport->GetCachedGeometry();
	
	const FVector2D LocalCenter = ViewportGeometry.GetLocalSize() * 0.5f;
	return ViewportGeometry.LocalToAbsolute(LocalCenter);
}

void UERNMinimapWidget::RefreshPlayerMarkers()
{
	if (PlayerMarkerLayer == nullptr || PlayerMarkerWidgetClass == nullptr || GetWorld() == nullptr)
	{
		return;
	}
	
	if (IconDataAsset == nullptr)
	{
		UE_LOG(LogTemp,Warning,TEXT("UERNMinimapWidget IconDataAsset is null"));
		return;
	}
	
	AGameStateBase* GameState = GetWorld()->GetGameState();
	if (GameState == nullptr)
	{
		return;
	}
	
	TSet<APawn*> CurrentPlayerPawns;
	
	// PlayerState의 PC가 조작하는 Pawn을 찾아냄. 틱 주기 * 플레이어 수 만큼 반복됨.
	for (APlayerState* CurrentPlayerState : GameState->PlayerArray)
	{
		if (CurrentPlayerState == nullptr)
		{
			continue;
		}
		
		AERNPlayerState* ERNPlayerState = Cast<AERNPlayerState>(CurrentPlayerState);
		if (ERNPlayerState == nullptr)
		{
			continue;
		}
		
		APawn* CurrentPlayerPawn = ERNPlayerState->GetPawn();
		if (IsValid(CurrentPlayerPawn) == false)
		{
			continue;
		}
		
		CurrentPlayerPawns.Add(CurrentPlayerPawn);
		
		UMinimapPlayerMarkerWidget* PlayerMarkerWidget  = nullptr;
		
		// 캐릭터에 마커 클래스 연동
		if (TObjectPtr<UMinimapPlayerMarkerWidget>* FoundMarkerWidgetPtr = PlayerMarkerWidgets.Find(CurrentPlayerPawn))
		{
			PlayerMarkerWidget = FoundMarkerWidgetPtr->Get();
		}
		
		// 존재하지 않으면 생성 먼저
		if (IsValid(PlayerMarkerWidget) == false)
		{
			PlayerMarkerWidget = CreateWidget<UMinimapPlayerMarkerWidget>(this, PlayerMarkerWidgetClass);
			if (PlayerMarkerWidget == nullptr)
			{
				continue;
			}
		
			PlayerMarkerWidget->SetPlayerPawn(CurrentPlayerPawn);
			PlayerMarkerWidgets.Add(CurrentPlayerPawn, PlayerMarkerWidget);
		
			if (UCanvasPanelSlot* PlayerMarkerCanvasSlot = PlayerMarkerLayer->AddChildToCanvas(PlayerMarkerWidget))
			{
				PlayerMarkerCanvasSlot ->SetAutoSize(true);
				PlayerMarkerCanvasSlot ->SetAlignment(FVector2D(0.5f, 0.5f));
			}
		}
		
		// 마커 최신화
		UTexture2D* PlayerMarkerIconTexture = IconDataAsset->FindIconTexture(ERNPlayerState->GetMinimapPlayerMarkerIconType());
		
		PlayerMarkerWidget->SetIconTexture(PlayerMarkerIconTexture);
		
		const FVector2D PlayerMapPosition = WorldToMapPosition(CurrentPlayerPawn->GetActorLocation());
		const float PlayerMapYaw = WorldYawToMapAngle(CurrentPlayerPawn->GetActorRotation().Yaw);
			
		PlayerMarkerWidget->UpdateMarker(PlayerMapPosition, PlayerMapYaw);
	}

	// 플레이어 마커의 존재가 유효하지 않으면 제거.
	for (auto PlayerMarkerIter = PlayerMarkerWidgets.CreateIterator(); PlayerMarkerIter; ++PlayerMarkerIter)
	{
		APawn* TrackedPlayerPawn  = PlayerMarkerIter.Key();
		UMinimapPlayerMarkerWidget* TrackedMarkerWidget  = PlayerMarkerIter.Value();
		
		const bool bHasInvalidPawn = IsValid(TrackedPlayerPawn) == false;
		const bool bHasMissingPawn = CurrentPlayerPawns.Contains(TrackedPlayerPawn) == false;
		const bool bHasInvalidMarkerWidget = IsValid(TrackedMarkerWidget) == false;

		// Pawn 제거 || 수집된 플레이어 목록에 없음(플레이어 캐릭터가 아닌 클래스에 빙의중) || 위젯이 유효하지 않음 
		if (bHasInvalidPawn || bHasMissingPawn || bHasInvalidMarkerWidget)
		{
			if (TrackedMarkerWidget)
			{
				TrackedMarkerWidget->RemoveFromParent();
			}

			PlayerMarkerIter.RemoveCurrent();
		}
	}
}

float UERNMinimapWidget::WorldYawToMapAngle(float WorldYaw) const
{
	return FRotator::NormalizeAxis(-(90.f - WorldYaw + 180.f));
}

void UERNMinimapWidget::StartDynamicMinimapRefresh()
{
	RefreshDynamicMinimapElements();
	
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	
	// 타이머 생성. RefreshDynamicMinimapElements 에서 자기장, 플레이어 위치 최신화
	World->GetTimerManager().SetTimer(DynamicMinimapRefreshTimerHandle, this, &UERNMinimapWidget::RefreshDynamicMinimapElements, DynamicMinimapRefreshInterval, true);
}

void UERNMinimapWidget::StopDynamicMinimapRefresh()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	
	World->GetTimerManager().ClearTimer(DynamicMinimapRefreshTimerHandle);
}

// 지도 조작 관련

FReply UERNMinimapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 좌클릭 : 미니맵 잡기
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		SetKeyboardFocus();
		
		bIsDraggingMap = true;
		LastDragScreenPosition = InMouseEvent.GetScreenSpacePosition();
		
		return FReply::Handled().CaptureMouse(TakeWidget());
	}
	
	// 휠 클릭 : 핀
	if (InMouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		if (MapContent == nullptr)
		{
			return FReply::Handled();
		}
		
		const FVector2D MapPosition = MapContent->GetCachedGeometry().AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		
		const FVector WorldLocation = MapToWorldPosition(MapPosition);
		
		if (AERNPlayerController* PC = Cast<AERNPlayerController>(GetOwningPlayer()))
		{
			PC->Server_RequestCreateMinimapPin(WorldLocation);
		}
		
		return FReply::Handled();
	}
	
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UERNMinimapWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	//드래그 종료
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsDraggingMap)
	{
		bIsDraggingMap = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}


FReply UERNMinimapWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDraggingMap == false)
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}
	
	AddMapPanOffsetByScreenDelta(InMouseEvent.GetScreenSpacePosition());
	
	return FReply::Handled();
}

FReply UERNMinimapWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 휠 줌인 & 줌아웃
	const float WheelDelta = InMouseEvent.GetWheelDelta();
	
	if (FMath::IsNearlyZero(WheelDelta))
	{
		return FReply::Handled();
	}
	
	const float ZoomMultiplier = FMath::Pow(MouseWheelZoomMultiplier, WheelDelta);
	SetMapZoomAtScreenPosition(CurrentMapZoom * ZoomMultiplier, InMouseEvent.GetScreenSpacePosition());
	
	return FReply::Handled();
}

void UERNMinimapWidget::ApplyMapTransform()
{
	if (MapContent == nullptr)
	{
		return;
	}
	
	FWidgetTransform MapTransform;
	MapTransform.Translation = CurrentMapPanOffset;
	MapTransform.Scale = FVector2D(CurrentMapZoom, CurrentMapZoom);
	
	MapContent->SetRenderTransformPivot(FVector2D::ZeroVector);
	MapContent->SetRenderTransform(MapTransform);
}

void UERNMinimapWidget::SetMapZoomAtScreenPosition(float NewZoom, const FVector2D& ScreenPosition)
{
	if (MapViewport == nullptr)
	{
		return;
	}
	
	const float OldZoom = CurrentMapZoom;
	const float ClampedNewZoom = FMath::Clamp(NewZoom, MinMapZoom, MaxMapZoom);
	
	if (FMath::IsNearlyEqual(OldZoom, ClampedNewZoom))
	{
		return;
	}
	
	const FVector2D MouseLocalPosition = MapViewport->GetCachedGeometry().AbsoluteToLocal(ScreenPosition);
	
	const float ZoomRatio = ClampedNewZoom / OldZoom;
	
	CurrentMapPanOffset = MouseLocalPosition - ((MouseLocalPosition - CurrentMapPanOffset) * ZoomRatio);
	
	CurrentMapZoom = ClampedNewZoom;
	
	ClampMapPanOffset();
	ApplyMapTransform();
	ApplyMarkerRenderScale();
}

void UERNMinimapWidget::AddMapPanOffsetByScreenDelta(const FVector2D& CurrentScreenPosition)
{
	if (MapViewport == nullptr)
	{
		return;
	}
	
	const FGeometry& ViewportGeometry = MapViewport->GetCachedGeometry();
	
	const FVector2D LastLocalPosition = ViewportGeometry.AbsoluteToLocal(LastDragScreenPosition);
	const FVector2D CurrentLocalPosition = ViewportGeometry.AbsoluteToLocal(CurrentScreenPosition);
	
	AddMapPanOffsetByDelta(CurrentLocalPosition - LastLocalPosition);

	LastDragScreenPosition = CurrentScreenPosition;
}

void UERNMinimapWidget::AddMapPanOffsetByDelta(const FVector2D& Delta)
{
	CurrentMapPanOffset += Delta;
	ClampMapPanOffset();
	ApplyMapTransform();
}

void UERNMinimapWidget::StartGamepadMoveRefresh()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(GamepadMoveTimerHandle, this, &UERNMinimapWidget::RefreshGamepadMove, 0.01f, true);
	}
}

void UERNMinimapWidget::StopGamepadMoveRefresh()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GamepadMoveTimerHandle);
	}
	GamepadPanInput = FVector2D::ZeroVector;
}

void UERNMinimapWidget::RefreshGamepadMove()
{
	if (GamepadPanInput.IsNearlyZero())
	{
		return;
	}
	
	AddMapPanOffsetByDelta(GamepadPanInput * GamepadMoveSpeed * 0.01f);
}

void UERNMinimapWidget::ClampMapPanOffset()
{
	if (MapViewport == nullptr)
	{
		return;
	}
	
	const FVector2D ViewportSize = MapViewport->GetCachedGeometry().GetLocalSize();
	const FVector2D ScaledMapSize = MapSize * CurrentMapZoom;
	
	auto ClampAxis = [](float Offset, float ViewSize, float ContentSize)
	{
		if (ContentSize <= ViewSize)
		{
			return (ViewSize - ContentSize) * 0.5f;
		}
		
		return FMath::Clamp(Offset, ViewSize - ContentSize, 0.f);
	};
	
	CurrentMapPanOffset.X = ClampAxis(CurrentMapPanOffset.X, ViewportSize.X, ScaledMapSize.X);
	CurrentMapPanOffset.Y = ClampAxis(CurrentMapPanOffset.Y, ViewportSize.Y, ScaledMapSize.Y);
	
}

float UERNMinimapWidget::GetMarkerRenderScale() const
{
	if (FMath::IsNearlyZero(CurrentMapZoom))
	{
		return 1.f;
	}
	
	return 1.f / CurrentMapZoom;
}

void UERNMinimapWidget::ApplyMarkerRenderScale()
{
	const float MarkerRenderScale = GetMarkerRenderScale();
	
	if (MarkerLayer)
	{
		for (int32 ChildIndex = 0; ChildIndex < MarkerLayer->GetChildrenCount(); ++ChildIndex)
		{
			if (UWidget* ChildWidget = MarkerLayer->GetChildAt(ChildIndex))
			{
				ChildWidget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
				ChildWidget->SetRenderScale(FVector2D(MarkerRenderScale, MarkerRenderScale));
			}
		}
	}
	
	if (PlayerMarkerLayer)
	{
		for (int32 ChildIndex = 0; ChildIndex < PlayerMarkerLayer->GetChildrenCount(); ++ChildIndex)
		{
			if (UWidget* ChildWidget = PlayerMarkerLayer->GetChildAt(ChildIndex))
			{
				ChildWidget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
				ChildWidget->SetRenderScale(FVector2D(MarkerRenderScale, MarkerRenderScale));
			}
		}
	}
	
}
