// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ERNSkillCoolSlot.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Components/Image.h"

namespace
{
	// 머티리얼 그래프에 만든 파라미터 이름과 정확히 일치해야 한다.
	const FName SkillIconTextureParameterName(TEXT("SkillIconTexture"));
	const FName CooldownPercentParameterName(TEXT("CooldownPercent"));
}

void UERNSkillCoolSlot::InitializeSkillCoolSlot(UAbilitySystemComponent* InAbilitySystemComponent,
	UTexture2D* InSkillIconTexture, const FGameplayTagContainer& InCooldownTags)
{
	AbilitySystemComponent = InAbilitySystemComponent;
	SkillIconTexture = InSkillIconTexture;
	CooldownTags = InCooldownTags;

	// 호출 시점에 위젯이 이미 Construct된 상태라면 즉시 화면에 반영한다.
	CacheIconDynamicMaterial();
	ApplySkillIconTexture();
	UpdateCooldownVisual();
	
	// 표시 복구
	SetVisibility(SkillIconTexture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UERNSkillCoolSlot::ClearSkillCoolSlot()
{
	AbilitySystemComponent = nullptr;
	SkillIconTexture = nullptr;
	CooldownTags.Reset();

	// 새 스킬이 연결될 때까지 이전 캐릭터 아이콘을 숨긴다.
	SetVisibility(ESlateVisibility::Collapsed);

	if (IconDynamicMaterial)
	{
		IconDynamicMaterial->SetScalarParameterValue(CooldownPercentParameterName, 1.0f);
	}
}

void UERNSkillCoolSlot::NativeConstruct()
{
	Super::NativeConstruct();
	
	// BP에서 설정한 머티리얼 인스턴스를 슬롯 전용 MID로 가져온다.
	CacheIconDynamicMaterial();

	// InitializeSkillCoolSlot이 NativeConstruct보다 먼저 호출된 경우에도
	// 정상적으로 이미지와 진행도가 반영되도록 다시 적용한다.
	ApplySkillIconTexture();
	UpdateCooldownVisual();
}

void UERNSkillCoolSlot::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	UpdateCooldownVisual();
}

void UERNSkillCoolSlot::CacheIconDynamicMaterial()
{
	if (IconDynamicMaterial || !SkillIconImage)
	{
		return;
	}

	/**
	 * BP의 SkillIconImage Brush에 머티리얼 인스턴스가 설정되어 있어야 한다.
	 * 텍스처만 넣어두면 GetDynamicMaterial은 nullptr을 반환한다.
	 */
	IconDynamicMaterial = SkillIconImage->GetDynamicMaterial();
}

void UERNSkillCoolSlot::ApplySkillIconTexture()
{
	if (!IconDynamicMaterial || !SkillIconTexture)
	{
		return;
	}

	IconDynamicMaterial->SetTextureParameterValue(SkillIconTextureParameterName, SkillIconTexture);
}

void UERNSkillCoolSlot::UpdateCooldownVisual()
{
	if (!IconDynamicMaterial)
	{
		return;
	}

	IconDynamicMaterial->SetScalarParameterValue(CooldownPercentParameterName, CalculateCooldownPercent());
}

float UERNSkillCoolSlot::CalculateCooldownPercent() const
{
	// ASC 또는 태그가 아직 설정되지 않았다면 사용 가능한 상태로 표시한다.
	if (!AbilitySystemComponent.IsValid() || CooldownTags.IsEmpty())
	{
		return 1.0f;
	}

	/**
	 * SkillBase::ApplyCooldown에서 GE에 DynamicGrantedTags를 추가했다.
	 * 따라서 ASC에서 해당 태그를 보유한 활성 GE를 찾으면
	 * 실제로 적용 중인 쿨타임의 남은 시간과 전체 시간을 얻을 수 있다.
	 */
	const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTags);

	const TArray<TPair<float, float>> RemainingAndDurationArray = AbilitySystemComponent->GetActiveEffectsTimeRemainingAndDuration(Query);

	float RemainingTime = 0.0f;
	float Duration = 0.0f;

	/**
	 * 일반적으로 같은 슬롯에는 쿨타임 GE가 하나만 존재한다.
	 * 혹시 여러 개가 발견되더라도 가장 오래 남은 쿨타임을 표시한다.
	 */
	for (const TPair<float, float>& RemainingAndDuration : RemainingAndDurationArray)
	{
		if (RemainingAndDuration.Key > RemainingTime)
		{
			RemainingTime = RemainingAndDuration.Key;
			Duration = RemainingAndDuration.Value;
		}
	}

	// 활성 쿨타임 GE가 없으면 전체 아이콘을 밝게 표시한다.
	if (RemainingTime <= 0.0f || Duration <= KINDA_SMALL_NUMBER)
	{
		return 1.0f;
	}

	/**
	 * 쿨타임 시작: RemainingTime == Duration
	 * 결과: 1 - 1 == 0
	 *
	 * 쿨타임 종료: RemainingTime == 0
	 * 결과: 1 - 0 == 1
	 */
	return FMath::Clamp(1.0f - (RemainingTime / Duration), 0.0f, 1.0f);
}
