// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ERNDamageTextActor.h"
#include "UI/ERNDamageTextWidget.h"
#include "Components/WidgetComponent.h"

AERNDamageTextActor::AERNDamageTextActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false; // 로컬 전용

	WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	WidgetComponent->SetWidgetSpace(EWidgetSpace::Screen); // 빌보드
	WidgetComponent->SetDrawAtDesiredSize(true);
	WidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootComponent = WidgetComponent;
}

void AERNDamageTextActor::BeginPlay()
{
	Super::BeginPlay();

	if (DamageTextWidgetClass)
	{
		WidgetComponent->SetWidgetClass(DamageTextWidgetClass);
	}
}

void AERNDamageTextActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ElapsedTime += DeltaTime;

	// 위로 이동
	FVector NewLocation = GetActorLocation();
	NewLocation.Z += FloatSpeed * DeltaTime;
	SetActorLocation(NewLocation);

	// 페이드아웃 + 스케일 축소 (처음엔 크게, 점점 작아짐)
	const float Progress = Duration > 0.f ? FMath::Clamp(ElapsedTime / Duration, 0.f, 1.f) : 1.f;
	const float Alpha = 1.f - Progress;
	const float CurrentScale = FMath::Lerp(StartScale, EndScale, Progress);
	if (UUserWidget* Widget = WidgetComponent->GetWidget())
	{
		Widget->SetRenderScale(FVector2D(CurrentScale, CurrentScale));
		Widget->SetRenderOpacity(Alpha * StartOpacity);
	}

	// 지속 시간 끝나면 파괴
	if (ElapsedTime >= Duration)
	{
		Destroy();
	}
}

void AERNDamageTextActor::Initialize(float InDamage)
{
	AccumulatedDamage = InDamage;
	ElapsedTime = 0.f;
	UpdateDisplay();
}

void AERNDamageTextActor::AddDamage(float InDamage)
{
	// 누적 + 수명/스케일 리셋 (다시 크게 떠서 떠오르고 작아짐)
	AccumulatedDamage += InDamage;
	ElapsedTime = 0.f;
	UpdateDisplay();
}

void AERNDamageTextActor::UpdateDisplay()
{
	// 누적 데미지 크기에 따라 색상 결정 (임계값 이상 빨강, 아니면 주황)
	const FLinearColor Color = (AccumulatedDamage >= HighDamageThreshold) ? HighDamageColor : NormalColor;

	if (UERNDamageTextWidget* Widget = Cast<UERNDamageTextWidget>(WidgetComponent->GetWidget()))
	{
		// 첫 프레임/재팝 팝핑 방지를 위해 시작 스케일 즉시 적용
		Widget->SetRenderScale(FVector2D(StartScale, StartScale));
		Widget->SetDamageText(AccumulatedDamage, Color);
	}
}
