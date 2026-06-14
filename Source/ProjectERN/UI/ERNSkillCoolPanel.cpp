// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ERNSkillCoolPanel.h"

#include "AbilitySystemComponent.h"
#include "ERNSkillCoolSlot.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GAS/ERNGameplayTags.h"
#include "UI/ERNUIManagerSubsystem.h"
#include "Engine/LocalPlayer.h"

void UERNSkillCoolPanel::NativeConstruct()
{
	Super::NativeConstruct();

	// 현재 캐릭터 기준으로 슬롯을 다시 연결
	RefreshFromCurrentCharacter();

	// 옵션/메뉴 등 전체화면 UI가 열리면 함께 숨겨지도록 UI 매니저 상태 변경을 구독
	if (const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>())
		{
			UIManager->OnUIStateChanged.AddUniqueDynamic(this, &UERNSkillCoolPanel::HandleUIStateChanged);
			HandleUIStateChanged(UIManager->GetActiveUIType()); // 초기 상태 반영
		}
	}
}

void UERNSkillCoolPanel::NativeDestruct()
{
	// 위젯이 제거된 뒤 타이머가 다시 호출되지 않도록 정리한다.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InitializeRetryTimerHandle);
	}

	// UI 매니저 구독 해제
	if (const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>())
		{
			UIManager->OnUIStateChanged.RemoveDynamic(this, &UERNSkillCoolPanel::HandleUIStateChanged);
		}
	}

	Super::NativeDestruct();
}

void UERNSkillCoolPanel::HandleUIStateChanged(EERNUIType UIType)
{
	if (UIType != EERNUIType::None)
	{
		SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UERNSkillCoolPanel::TryInitializeSkillSlots()
{
	AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(GetOwningPlayerPawn());

	/**
	 * Controller의 BeginPlay 시점에는 Pawn 또는 ASC가 아직 준비되지
	 * 않았을 수 있다. 이 경우 잠시 기다린 뒤 다시 시도한다.
	 */
	if (!PlayerCharacter || !PlayerCharacter->GetAbilitySystemComponent())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				InitializeRetryTimerHandle,
				this,
				&UERNSkillCoolPanel::TryInitializeSkillSlots,
				0.2f,
				false);
		}

		return;
	}

	UAbilitySystemComponent* ASC = PlayerCharacter->GetAbilitySystemComponent();

	bool bNormalSkillInitialized = false;
	bool bUltimateSkillInitialized = false;

	/**
	 * ASC에 등록된 모든 Ability Spec을 검사한다.
	 * 각 Spec의 Ability에는 Normal 또는 Ultimate 태그가 자동으로 붙어 있다.
	 */
	for (const FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
	{
		const UERNGA_SkillBase* SkillAbility = Cast<UERNGA_SkillBase>(AbilitySpec.Ability);

		// SkillBase 계열이 아니면 건너뛴다.
		if (!SkillAbility)
		{
			continue;
		}

		const FGameplayTagContainer& AbilityTags = SkillAbility->GetAssetTags();

		if (!bNormalSkillInitialized && AbilityTags.HasTagExact(TAG_Ability_Skill_Normal))
		{
			NormalSkillSlot->InitializeSkillCoolSlot(
				ASC,
				SkillAbility->GetSkillIconTexture(),
				SkillAbility->GetSkillCooldownTags());

			bNormalSkillInitialized = true;
		}

		if (!bUltimateSkillInitialized && AbilityTags.HasTagExact(TAG_Ability_Skill_Ultimate))
		{
			UltimateSkillSlot->InitializeSkillCoolSlot(
				ASC,
				SkillAbility->GetSkillIconTexture(),
				SkillAbility->GetSkillCooldownTags());

			bUltimateSkillInitialized = true;
		}

		if (bNormalSkillInitialized && bUltimateSkillInitialized)
		{
			break;
		}
	}

	/**
	 * ASC는 준비됐지만 Ability Grant가 아직 끝나지 않았을 수도 있다.
	 * 두 스킬을 모두 찾지 못했다면 잠시 후 다시 검색한다.
	 */
	if ((!bNormalSkillInitialized || !bUltimateSkillInitialized) &&
		GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			InitializeRetryTimerHandle,
			this,
			&UERNSkillCoolPanel::TryInitializeSkillSlots,
			0.2f,
			false);
	}
}

void UERNSkillCoolPanel::RefreshFromCurrentCharacter()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InitializeRetryTimerHandle);
	}

	if (NormalSkillSlot)
	{
		NormalSkillSlot->ClearSkillCoolSlot();
	}

	if (UltimateSkillSlot)
	{
		UltimateSkillSlot->ClearSkillCoolSlot();
	}

	TryInitializeSkillSlots();
}
