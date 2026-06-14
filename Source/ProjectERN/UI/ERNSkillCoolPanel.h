// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ERNSkillCoolPanel.generated.h"

enum class EERNUIType : uint8;
class UERNSkillCoolSlot;

/**
 * NormalSkill과 UltimateSkill 슬롯을 초기화하는 부모 패널.
 *
 * ASC에 등록된 Ability를 검색하여 각 슬롯에
 * ASC, 아이콘 텍스처, 쿨타임 태그를 전달한다.
 */
UCLASS()
class PROJECTERN_API UERNSkillCoolPanel : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// BP 내부 슬롯 이름과 정확히 일치해야 한다.
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UERNSkillCoolSlot> NormalSkillSlot;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UERNSkillCoolSlot> UltimateSkillSlot;

private:
	// Pawn과 ASC 준비 여부를 확인하고 슬롯을 초기화, 준비가 끝나지 않았다면 잠시 후 다시 호출
	void TryInitializeSkillSlots();

	// UI 매니저의 상호작용 UI 상태 변경 시 숨김/표시 처리 (옵션/메뉴 등이 열리면 숨김)
	UFUNCTION()
	void HandleUIStateChanged(EERNUIType UIType);

	FTimerHandle InitializeRetryTimerHandle;
	
public:
	// 현재 로컬 플레이어가 조종 중인 Pawn을 기준으로 슬롯을 다시 연결한다.
	void RefreshFromCurrentCharacter();
};
