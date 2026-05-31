// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "ERNSkillCoolSlot.generated.h"

class UAbilitySystemComponent;
class UImage;
class UMaterialInstanceDynamic;
class UTexture2D;

/**
 * 스킬 하나의 쿨타임을 표시하는 공용 UI 슬롯.
 *
 * NormalSkill과 UltimateSkill 슬롯이 동일한 클래스를 재사용한다.
 * 실제 스킬 탐색은 부모 패널이 담당하고,
 * 이 위젯은 전달받은 쿨타임 태그를 이용해 ASC에서 남은 시간을 조회한다.
 */
UCLASS()
class PROJECTERN_API UERNSkillCoolSlot : public UUserWidget
{
	GENERATED_BODY()
	
public:
	/**
	 * 슬롯에 표시할 스킬 정보를 설정한다.
	 *
	 * @param InAbilitySystemComponent 쿨타임 GE를 검색할 플레이어의 ASC
	 * @param InSkillIconTexture       SkillBase에 설정된 스킬 아이콘
	 * @param InCooldownTags           Normal 또는 Ultimate 쿨타임 태그
	 */
	UFUNCTION(BlueprintCallable, Category="ERN|Skill|Cooldown")
	void InitializeSkillCoolSlot(
		UAbilitySystemComponent* InAbilitySystemComponent,
		UTexture2D* InSkillIconTexture,
		const FGameplayTagContainer& InCooldownTags);
	
	// 캐릭터 변경 시 기존 ASC와 스킬 정보를 제거한다.
	void ClearSkillCoolSlot();
	
protected:
	virtual void NativeConstruct() override;
	
	// 쿨타임 진행도를 부드럽게 갱신하기 위해 매 프레임 호출
	virtual void NativeTick(
		const FGeometry& MyGeometry,
		float InDeltaTime) override;

	
	// SkillIconImage
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> SkillIconImage;

private:
	// BP의 Image Brush에 설정된 머티리얼로부터 동적 인스턴스를 가져온다.
	void CacheIconDynamicMaterial();

	// 동적 머티리얼에 현재 스킬 아이콘을 전달한다.
	void ApplySkillIconTexture();

	// 현재 쿨타임 진행도를 머티리얼 파라미터에 전달한다.
	void UpdateCooldownVisual();

	/**
	 * 0.0: 쿨타임 시작
	 * 1.0: 사용 가능
	 */
	float CalculateCooldownPercent() const;

	// 플레이어 ASC는 다른 객체가 소유하므로 약한 참조로 보관한다.
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	// 조회할 공용 쿨타임 태그. Normal과 Ultimate 슬롯에서 각각 다르게 설정된다.
	FGameplayTagContainer CooldownTags;

	// 런타임에 적용할 스킬 아이콘.
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> SkillIconTexture;

	// 각 슬롯이 독립적으로 파라미터를 변경할 수 있도록 생성되는 MID.
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> IconDynamicMaterial;
};
