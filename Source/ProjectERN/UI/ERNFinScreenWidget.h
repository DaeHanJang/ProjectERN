// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ERNFinScreenWidget.generated.h"

class UTextBlock;
class UWidgetAnimation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFinScreenContinue);

/**
 * 승리 시 WBP_Victory 이전에 표시되는 "Fin (?)" 연출 위젯.
 *
 * 진행 순서:
 *   1. (배경 이미지는 WBP에서 표시) "Fin" 페이드 인
 *   2. "Fin" 완료 후 바로 뒤 "(?)" 페이드 인 → "Fin (?)"
 *   3. 완료 후 "Space" 프롬프트 깜빡임 + Space 입력 활성화
 *   4. Space 입력 시 OnContinue 브로드캐스트 후 자신을 제거 (→ PC가 승리 배너 표시)
 *
 * 애니메이션이 누락되어도 다음 단계로 진행되어 Space 활성화에 도달하므로 진행 막힘이 없도록 설계.
 */
UCLASS()
class PROJECTERN_API UERNFinScreenWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UERNFinScreenWidget(const FObjectInitializer& ObjectInitializer);

	// Space 입력으로 연출 종료 시 호출 (PC가 바인딩하여 승리 배너 표시)
	UPROPERTY(BlueprintAssignable, Category = "ERN|FinScreen")
	FOnFinScreenContinue OnContinue;

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// === 위젯 바인딩 (WBP에서 동일 이름으로 생성) ===
	// "(?)" 텍스트 — 시작 시 숨김, Fin 페이드 후 표시/페이드 인
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuestionText;

	// "Space" 프롬프트 — Fin (?) 완성 후 깜빡임 시작
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SpacePromptText;

	// === 애니메이션 (WBP에서 동일 이름으로 생성) ===
	// "Fin" 페이드 인
	UPROPERTY(meta = (BindWidgetAnimOptional), Transient)
	TObjectPtr<UWidgetAnimation> FinFadeIn;

	// "(?)" 페이드 인
	UPROPERTY(meta = (BindWidgetAnimOptional), Transient)
	TObjectPtr<UWidgetAnimation> QuestionFadeIn;

	// "Space" 프롬프트 깜빡임 (무한 루프 재생)
	UPROPERTY(meta = (BindWidgetAnimOptional), Transient)
	TObjectPtr<UWidgetAnimation> SpaceBlink;

private:
	// 애니메이션 시퀀스 콜백
	UFUNCTION()
	void HandleFinFadeFinished();

	UFUNCTION()
	void HandleQuestionFadeFinished();

	// Space 프롬프트 깜빡임 시작 + 입력 활성화
	void StartSpacePrompt();

	// Space 입력 확정 처리 (1회만)
	void Continue();

	// Space 입력 활성화 여부 (연출 완료 후 true)
	bool bSpaceEnabled = false;

	// 중복 진행 방지
	bool bContinued = false;
};
