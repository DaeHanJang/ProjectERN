// 상점 메인 위젯 - 소지 골드 표시 및 구매 결과 피드백을 C++에서 자동 처리

#pragma once

#include "CoreMinimal.h"
#include "UI/ERNInteractableWidget.h"
#include "ERNShopMainWidget.generated.h"

class UTextBlock;
class UERNAttributeSet;
class UAbilitySystemComponent;
struct FOnAttributeChangeData;

/**
 * UERNShopMainWidget
 * 
 * 상점 메인 UI의 C++ 부모 클래스입니다.
 * 플레이어의 소지 골드(AttributeSet의 Gold)를 텍스트 블록에 자동 연동하고,
 * 골드가 변경될 때마다 실시간으로 업데이트합니다.
 * 
 * 블루프린트(WBP_ShopMain)의 부모 클래스를 이 클래스로 변경(Reparent)하면
 * GoldText 라는 이름의 TextBlock에 골드 값이 자동으로 표시됩니다.
 */
UCLASS()
class PROJECTERN_API UERNShopMainWidget : public UERNInteractableWidget
{
	GENERATED_BODY()

public:
	UERNShopMainWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 골드 표시 텍스트 블록 (블루프린트 위젯 이름을 GoldText로 맞춰야 자동 연동됨)
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* GoldText;

	// 골드 값이 변경될 때 호출되는 콜백
	void OnGoldChanged(const FOnAttributeChangeData& Data);

	// 현재 골드 값으로 텍스트 갱신
	UFUNCTION(BlueprintCallable, Category = "Shop|UI")
	void RefreshGoldDisplay();

private:
	// 어트리뷰트 변경 감지 해제를 위한 핸들
	FDelegateHandle GoldChangedDelegateHandle;

	// 캐싱된 어빌리티 시스템 컴포넌트 (Destruct 시 언바인딩용)
	UPROPERTY()
	UAbilitySystemComponent* CachedASC = nullptr;
};
