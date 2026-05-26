// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/World/ERNMinimapWidget.h"

#include "EngineUtils.h"
#include "MinimapMarkerWidget.h"
#include "MinimapPlayerMarkerWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "World/ERNMinimapSubsystem.h"
#include "World/ERNMinimapTargetPoint.h"
#include "World/Data/ERNMinimapIconDataAsset.h"
#include "Character/Player/ERNPlayerController.h"
#include "Character/Player/ERNPlayerState.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"


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

FReply UERNMinimapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 좌클릭 : 미니맵 잡기
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		SetKeyboardFocus();
		return FReply::Handled();
	}
	
	// 드래그 : 미니맵 이동

	// 휠 클릭 : 핀
	if (InMouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		if (MarkerLayer == nullptr)
		{
			return FReply::Handled();
		}
		
		const FVector2D MapPosition = MarkerLayer->GetCachedGeometry().AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		
		const FVector WorldLocation = MapToWorldPosition(MapPosition);
		
		if (AERNPlayerController* PC = Cast<AERNPlayerController>(GetOwningPlayer()))
		{
			PC->Server_RequestCreateMinimapPin(WorldLocation);
		}
		
		return FReply::Handled();
	}
	
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
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

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UERNMinimapWidget::NativeTick(const FGeometry& Ingeometry, float InDeltaTime)
{
	Super::NativeTick(Ingeometry, InDeltaTime);
	
	if (GetVisibility() != ESlateVisibility::Visible)
	{
		return;
	}
	
	RefreshPlayerMarkers();
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
	
	return FVector2D(NormalizedX * MapSize.X, (1 - NormalizedY) * MapSize.Y);
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
	const float WorldY = WorldMin.Y + (1.f - NormalizedY) * WorldSize.Y;
	
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
	
	// todo 병목 확인 필요, 최적화 여부 결정 필요
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
	return 90.f - WorldYaw;
}
