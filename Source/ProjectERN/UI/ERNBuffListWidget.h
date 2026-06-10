// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "ERNBuffListWidget.generated.h"

class UERNBuffIconWidget;
class AERNPlayerState;
class UHorizontalBox;

/**
 * 버프 아이콘들을 모아서 보여주는 리스트 컨테이너 위젯
 */
UCLASS()
class PROJECTERN_API UERNBuffListWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 특정 PlayerState에 바인딩하여 소모품 버프 이벤트를 수신 시작
	UFUNCTION(BlueprintCallable, Category = "Buff UI")
	void InitializeWithPlayerState(AERNPlayerState* PS);

protected:
	virtual void NativeDestruct() override;

	// 동적으로 버프 아이콘들이 담길 가로 컨테이너 (줄바꿈 없음)
	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* BuffContainerBox;

	// 버프 아이콘 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "Buff UI")
	TSubclassOf<UERNBuffIconWidget> BuffIconWidgetClass;

private:
	// 연결된 PlayerState
	TWeakObjectPtr<AERNPlayerState> WeakPS;

	// 버프 획득 콜백
	UFUNCTION()
	void OnConsumableBuffAdded(const FName& ItemID, float Duration);

	// 현재 활성화된 버프 위젯들 (같은 아이템 중복 방지용)
	UPROPERTY(Transient)
	TMap<FName, UERNBuffIconWidget*> ActiveBuffWidgets;
};
