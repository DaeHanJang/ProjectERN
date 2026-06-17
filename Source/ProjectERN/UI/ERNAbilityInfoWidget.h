#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ERNAbilityInfoWidget.generated.h"

class UTextBlock;

/**
 * 아이템 어빌리티 정보를 화면에 텍스트로 띄워주기 위한 전용 위젯
 */
UCLASS()
class PROJECTERN_API UERNAbilityInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 어빌리티 텍스트 설정
	void SetAbilityText(const FText& InText);

protected:
	// 블루프린트에서 반드시 'AbilityText' 라는 이름으로 TextBlock을 생성해야 합니다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> AbilityText;
};
