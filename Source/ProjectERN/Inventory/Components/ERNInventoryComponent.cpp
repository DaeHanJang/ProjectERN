// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inventory/Components/ERNInventoryComponent.h"

#include "Character/ERNCharacterBase.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GAS/ERNAttributeSet.h"
#include "Inventory/Item/ERNItemActor.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

UERNInventoryComponent::UERNInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	Inventory.SetOwner(this);
}

void UERNInventoryComponent::CopyInventoryFrom(const UERNInventoryComponent* Source)
{
	if (!Source)
	{
		return;
	}
	
	Inventory.CopyFrom(Source->GetInventory(), MaxSlotSize);
	
	RecalculateItemAbilities();
	
	for (const FInventoryItemEntry& Entry : Inventory.GetItems())
	{
		OnInventorySlotChanged.Broadcast(Entry);
	}
}

void UERNInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// 스냅샷 복원이 먼저 일어났으면(아이템 존재) 덮어쓰지 않음
	if (Inventory.GetItems().Num() == 0)
	{
		Inventory.Init(MaxSlotSize);
	}
	else
	{
		// 스냅샷으로 복원된 아이템이 있다면 어빌리티 다시 적용
		RecalculateItemAbilities();
	}
}

void UERNInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UERNInventoryComponent, Inventory);
}

UItemManagerSubsystem* UERNInventoryComponent::GetItemManager() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GI = World->GetGameInstance())
		{
			return GI->GetSubsystem<UItemManagerSubsystem>();
		}
	}
	
	return nullptr;
}

void UERNInventoryComponent::Server_AddItem_Implementation(AERNItemActor* ItemActor)
{
	// 매개 변수 유효성 검사
	if (!ItemActor)
	{
		return;
	}
	
	FItemRuntimeState& ItemRuntimeState = ItemActor->GetItemRuntimeState();
	// ItemActor의 ItemRuntimeState가 유효하지 않을 때
	if (!ItemRuntimeState.IsValid())
	{
		return;
	}
	
	const UItemManagerSubsystem* ItemManager = GetItemManager();
	// 서버에서 ItemID 유효성 검사
	if (!ItemManager || !ItemManager->ItemValid(ItemRuntimeState.GetItemID()))
	{
		return;
	}
	
	TArray<FInventoryItemEntry> ChangedSlots;
	// Inventory에 넣기 요청
	const FERNItemTable* ItemTable = ItemManager->FindItemRow(ItemRuntimeState.GetItemID());
	if (!Inventory.AddItem(ItemRuntimeState, MaxSlotSize, ItemTable->MaxStackSize, ChangedSlots))
	{
		return;
	}
	
	RecalculateItemAbilities();
	
	// 리슨 서버 인벤토리 변경 이벤트 발신 (UI 갱신)
	for (const FInventoryItemEntry& ChangedSlot : ChangedSlots)
	{
		OnInventorySlotChanged.Broadcast(ChangedSlot);
	}
	
	Inventory.LogInventory();
	
	if (ItemRuntimeState.GetQuantity() <= 0)
	{
		ItemActor->Destroy();
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Item: %s, Quantity: %d, Ability: %d"), *ItemRuntimeState.GetItemID().ToString(), ItemRuntimeState.GetQuantity(), static_cast<int32>(ItemRuntimeState.GetItemAbility()));
}

void UERNInventoryComponent::Server_RemoveItem_Implementation(const int32 SlotIndex, const int32 Count)
{
	// 매개 변수 유효성 검사
	if (!Inventory.GetItems().IsValidIndex(SlotIndex))
	{
		return;
	}
	if (Count <= 0 || Count > GetItemQuantity(SlotIndex))
	{
		return;
	}
	
	// 리슨 서버 UI 갱신용 갱신된 인벤토리 슬롯 배열
	FInventoryItemEntry ChangedSlot;
	// 인벤토리에서 제거 후 월드에 생성할 아이템 액터의 런타임 상태
	FItemRuntimeState DropItemRuntimeState;
	// 인벤토리에서 아이템 제거
	Inventory.RemoveItem(SlotIndex, Count, DropItemRuntimeState, ChangedSlot);
	
	const UItemManagerSubsystem* ItemManager = GetItemManager();
	// 서버에서 ItemID 유효성 검사
	if (!ItemManager || !ItemManager->ItemValid(DropItemRuntimeState.GetItemID()))
	{
		return;
	}
	const FERNItemTable* ItemTable = ItemManager->FindItemRow(DropItemRuntimeState.GetItemID());
	if (ItemTable->ItemType == EItemType::Equipable)
	{
		UE_LOG(LogTemp, Warning, TEXT("Item: %s, Quantity: %d, Ability: %d"), *DropItemRuntimeState.GetItemID().ToString(), DropItemRuntimeState.GetQuantity(), static_cast<int32>(DropItemRuntimeState.GetItemAbility()));
	}

	RecalculateItemAbilities();
	
	// 리슨 서버 전용 UI 갱신 이벤트
	OnInventorySlotChanged.Broadcast(ChangedSlot);
	
	Inventory.LogInventory();
	
	// 인벤토리에서 제거한 아이템 월드에 생성 
	const FVector& DropLocation = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * 50.0f;
	AActor* Item = GetItemManager()->SpawnItem(DropItemRuntimeState, DropLocation, GetOwner()->GetActorRotation());
	if (AERNItemActor* ERNItem = Cast<AERNItemActor>(Item))
	{
		ERNItem->Launch(GetOwner()->GetActorForwardVector());
	}
	
	UE_LOG(LogTemp, Warning, TEXT("DropItem: %s, Quantity: %d"), *DropItemRuntimeState.GetItemID().ToString(), DropItemRuntimeState.GetQuantity());
	UE_LOG(LogTemp, Warning, TEXT("CurrentItem: %s, Quantity: %d"), *Inventory.GetItems()[SlotIndex].GetItemID().ToString(), Inventory.GetItemQuantity(SlotIndex));
}

void UERNInventoryComponent::RecalculateItemAbilities()
{
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(GetOwner());
	if (!Character)
	{
		return;
	}

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// 기존 GE 버프 모두 해제
	for (const FActiveGameplayEffectHandle& Handle : ActiveItemAbilityHandles)
	{
		ASC->RemoveActiveGameplayEffect(Handle);
	}
	ActiveItemAbilityHandles.Empty();

	// 논-어트리뷰트 변수 초기화
	Character->AddGoldWeight(-Character->GetGoldWeight());
	Character->BonusLifestealFraction = 0.0f;

	const UItemManagerSubsystem* ItemManager = GetItemManager();
	if (!ItemManager)
	{
		return;
	}

	// 동적 GE 생성 (고정된 이름을 쓰면 이전 객체가 재사용되어 GAS 내부 참조 크래시가 발생하므로, 매번 고유한 임시 객체를 생성)
	UGameplayEffect* GE = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
	GE->DurationPolicy = EGameplayEffectDurationType::Infinite;

	// 인벤토리 전체를 순회하며 효과 누적
	for (int32 i = 0; i < Inventory.GetItems().Num(); ++i)
	{
		const FInventoryItemEntry& Entry = Inventory.GetItems()[i];
		const FItemRuntimeState& ItemState = Entry.GetItemRuntimeState();
		if (!ItemState.IsValid()) continue;

		const FERNItemTable* ItemRow = ItemManager->FindItemRow(ItemState.GetItemID());
		if (!ItemRow || ItemRow->ItemType != EItemType::Equipable) continue;

		EItemAbility Ability = ItemState.GetItemAbility();
		if (Ability == EItemAbility::None) continue;

		int32 Weight = static_cast<int32>(ItemRow->Grade) + 1;

		auto AddMod = [&](FGameplayAttribute Attribute, float Value)
		{
			int32 Idx = GE->Modifiers.AddDefaulted();
			FGameplayModifierInfo& ModInfo = GE->Modifiers[Idx];
			ModInfo.Attribute = Attribute;
			ModInfo.ModifierOp = EGameplayModOp::Additive;
			ModInfo.ModifierMagnitude = FScalableFloat(Value);
		};

		switch (Ability)
		{
		case EItemAbility::Health:
			AddMod(UERNAttributeSet::GetMaxHealthAttribute(), 20.0f * Weight);
			break;
		case EItemAbility::Attack:
			AddMod(UERNAttributeSet::GetAttackPowerAttribute(), 3.0f * Weight);
			break;
		case EItemAbility::HealthAndAttack:
			AddMod(UERNAttributeSet::GetMaxHealthAttribute(), 10.0f * Weight);
			AddMod(UERNAttributeSet::GetAttackPowerAttribute(), 1.0f * Weight);
			break;
		case EItemAbility::Stamina:
			AddMod(UERNAttributeSet::GetMaxStaminaAttribute(), 10.0f * Weight);
			break;
		case EItemAbility::Defence:
			AddMod(UERNAttributeSet::GetDefenseAttribute(), 3.0f * Weight);
			break;
		case EItemAbility::Gold:
			Character->AddGoldWeight(50.0f * Weight);
			break;
		case EItemAbility::Drain:
			Character->BonusLifestealFraction += 0.002f * Weight;
			break;
		case EItemAbility::HealthCurse:
			AddMod(UERNAttributeSet::GetMaxHealthAttribute(), -15.0f * Weight);
			AddMod(UERNAttributeSet::GetAttackPowerAttribute(), 4.0f * Weight);
			break;
		case EItemAbility::AttackCurse:
			AddMod(UERNAttributeSet::GetAttackPowerAttribute(), -4.0f * Weight);
			AddMod(UERNAttributeSet::GetDefenseAttribute(), 3.0f * Weight);
			AddMod(UERNAttributeSet::GetMaxHealthAttribute(), 20.0f * Weight);
			break;
		default:
			break;
		}
	}

	if (GE->Modifiers.Num() > 0)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectToSelf(GE, 1.0f, Context);
		if (Handle.WasSuccessfullyApplied())
		{
			ActiveItemAbilityHandles.Add(Handle);
		}
	}
}
