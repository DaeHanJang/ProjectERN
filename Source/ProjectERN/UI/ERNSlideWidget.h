// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ERNSlideWidget.generated.h"

class UEditableText;
class UButton;
class UTextBlock;
class USlider;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConfirmButtonClicked, const int32, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCancelButtonClicked, const int32, NewValue);

/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNSlideWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UERNSlideWidget(const FObjectInitializer& ObjectInitializer);

	// Getter
	FORCEINLINE const int32 GetValue() const { return Value; };
	
	// 위젯 초기화
	void InitSlideWidget(const FString& Text, const int32 MaxValue) const;
	
protected:
	virtual void NativeConstruct() override;
	
private:
	// 입력 텍스트 갱신 이벤트 핸들러
	UFUNCTION()
	void HandleEditableTextChanged(const FText& Text);
	UFUNCTION()
	void HandleEditableTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
	
	// 슬라이더 갱신 이벤트 핸들러
	UFUNCTION()
	void HandleSlider(float InValue);
	
	// 확인 버튼 클릭 이벤트 핸들러
	UFUNCTION()
	void HandleConfirmButtonClicked();
	
	// 취소 버튼 클릭 이벤트 핸들러
	UFUNCTION()
	void HandleCancelButtonClicked();
	
public:
	// 확인 버튼 클릭 이벤트
	FOnConfirmButtonClicked OnConfirmButtonClicked;
	
	// 취소 버튼 클릭 이벤트
	FOnCancelButtonClicked OnCancelButtonClicked;
	
private:
	// 내용 텍스트
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> SlideWidgetTextBlock;
	
	// 값 입력 텍스트
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UEditableText> SlideWidgetEditableText;
	
	// 최대 값 텍스트
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> MaxTextBlock;
	
	// 슬라이더
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<USlider> SlideWidgetSlider;
	
	// 확인 버튼
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> ConfirmButton;
	
	// 취소 버튼
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> CancelButton;
	
	// 값
	int32 Value = 0;
	
};
