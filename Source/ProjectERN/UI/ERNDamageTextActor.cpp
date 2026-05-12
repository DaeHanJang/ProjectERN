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

	// 페이드아웃
	const float Alpha = FMath::Clamp(1.f - (ElapsedTime / Duration), 0.f, 1.f);
	if (UUserWidget* Widget = WidgetComponent->GetWidget())
	{
		Widget->SetRenderOpacity(Alpha * StartOpacity);
	}

	// 지속 시간 끝나면 파괴
	if (ElapsedTime >= Duration)
	{
		Destroy();
	}
}

void AERNDamageTextActor::Initialize(float InDamage, FLinearColor InColor)
{
	if (UERNDamageTextWidget* Widget = Cast<UERNDamageTextWidget>(WidgetComponent->GetWidget()))
	{
		Widget->SetDamageText(InDamage, InColor);
	}
}
