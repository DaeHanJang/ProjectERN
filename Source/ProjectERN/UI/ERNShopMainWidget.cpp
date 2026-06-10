// 상점 메인 위젯 - 소지 골드 표시 및 실시간 갱신 구현

#include "UI/ERNShopMainWidget.h"
#include "Components/TextBlock.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GAS/ERNAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"

UERNShopMainWidget::UERNShopMainWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UERNShopMainWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 로컬 플레이어의 캐릭터에서 AttributeSet과 ASC를 가져와 골드 변경 감지를 등록합니다.
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;

	AProjectERNCharacter* PlayerChar = Cast<AProjectERNCharacter>(PC->GetPawn());
	if (!PlayerChar) return;

	UAbilitySystemComponent* ASC = PlayerChar->GetAbilitySystemComponent();
	if (!ASC) return;

	CachedASC = ASC;

	// Gold 어트리뷰트 변경 감지 등록
	GoldChangedDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(
		UERNAttributeSet::GetGoldAttribute()
	).AddUObject(this, &UERNShopMainWidget::OnGoldChanged);

	// 최초 1회 골드 표시
	RefreshGoldDisplay();
}

void UERNShopMainWidget::NativeDestruct()
{
	// 위젯이 파괴될 때 델리게이트 해제 (메모리 누수 방지)
	if (CachedASC)
	{
		CachedASC->GetGameplayAttributeValueChangeDelegate(
			UERNAttributeSet::GetGoldAttribute()
		).Remove(GoldChangedDelegateHandle);
		CachedASC = nullptr;
	}

	Super::NativeDestruct();
}

void UERNShopMainWidget::OnGoldChanged(const FOnAttributeChangeData& Data)
{
	// 골드 값이 변경될 때마다 자동 호출됩니다.
	RefreshGoldDisplay();
}

void UERNShopMainWidget::RefreshGoldDisplay()
{
	if (!GoldText) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;

	AProjectERNCharacter* PlayerChar = Cast<AProjectERNCharacter>(PC->GetPawn());
	if (!PlayerChar) return;

	UERNAttributeSet* AttributeSet = PlayerChar->GetAttributeSet();
	if (!AttributeSet) return;

	int32 CurrentGold = FMath::FloorToInt(AttributeSet->GetGold());
	GoldText->SetText(FText::AsNumber(CurrentGold));
}
