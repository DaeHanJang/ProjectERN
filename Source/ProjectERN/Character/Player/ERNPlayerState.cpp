// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Player/ERNPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GAS/ERNAttributeSet.h"
#include "Core/ERNGameState.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/Components/ERNEquipmentComponent.h"
#include "Core/ERNGameInstance.h"
#include "Inventory/Components/ERNInventoryComponent.h"

AERNPlayerState::AERNPlayerState()
{
	// 기본값 - 로비에서 설정됨
		CharacterType = ECharacterType::None;
	PlayerNickname = TEXT("Player");
	bIsReady = false;
}

// Replication 명단
void AERNPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AERNPlayerState, CharacterType);
	DOREPLIFETIME(AERNPlayerState, PlayerNickname);
	DOREPLIFETIME(AERNPlayerState, AccountLevel);
	DOREPLIFETIME(AERNPlayerState, bIsReady);

	DOREPLIFETIME(AERNPlayerState, PlayerNumber);

	DOREPLIFETIME(AERNPlayerState, bHasSnapshot);
	DOREPLIFETIME(AERNPlayerState, SavedLevel);
	DOREPLIFETIME(AERNPlayerState, SavedGold);
	DOREPLIFETIME(AERNPlayerState, SavedFlaskQuantity);
	DOREPLIFETIME(AERNPlayerState, SavedMaxFlaskQuantity);
	DOREPLIFETIME(AERNPlayerState, SavedInventory);
	DOREPLIFETIME(AERNPlayerState, SavedWeaponState);
	DOREPLIFETIME(AERNPlayerState, SavedConsumableState);
	DOREPLIFETIME(AERNPlayerState, KillCount);
	DOREPLIFETIME(AERNPlayerState, TotalDamageDealt);
	DOREPLIFETIME(AERNPlayerState, TotalLifestealHealed);
	DOREPLIFETIME(AERNPlayerState, TotalUltimateHealed);
	DOREPLIFETIME(AERNPlayerState, TotalBarrierBlocked);

	DOREPLIFETIME(AERNPlayerState, bIsInInstance);
	DOREPLIFETIME(AERNPlayerState, SavedFieldTransform);
}

void AERNPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	// SeamlessTravel 시 새로 spawn되는 PS에 옛 PS의 값을 옮겨줘야 GameMode가 올바른 캐릭터로 spawn함
	if (AERNPlayerState* NewPS = Cast<AERNPlayerState>(PlayerState))
	{
		NewPS->CharacterType = CharacterType;
		NewPS->PlayerNickname = PlayerNickname;
		NewPS->AccountLevel = AccountLevel;
		// bIsReady는 새 맵에선 의미 없으므로 복사 안 함

		// 런 진행 스냅샷 + 전과 누적도 새 PS로 이전 (Seamless Travel 보존)
		NewPS->bHasSnapshot = bHasSnapshot;
		NewPS->SavedLevel = SavedLevel;
		NewPS->SavedGold = SavedGold;
		NewPS->SavedFlaskQuantity = SavedFlaskQuantity;
		NewPS->SavedMaxFlaskQuantity = SavedMaxFlaskQuantity;
		NewPS->SavedInventory = SavedInventory;
		NewPS->SavedWeaponState = SavedWeaponState;
		NewPS->SavedConsumableState = SavedConsumableState;
		NewPS->KillCount = KillCount;
		NewPS->TotalDamageDealt = TotalDamageDealt;
		NewPS->TotalLifestealHealed = TotalLifestealHealed;
		NewPS->TotalUltimateHealed = TotalUltimateHealed;
		NewPS->TotalBarrierBlocked = TotalBarrierBlocked;
		
		// 미니맵에 사용될 플레이어 번호
		NewPS->PlayerNumber = PlayerNumber;
	}
}

void AERNPlayerState::SetNickname(const FString& Nickname)
{
	Server_SetNickname(Nickname);
}

void AERNPlayerState::Server_SetNickname_Implementation(const FString& Nickname)
{
	if (!Nickname.IsEmpty() && Nickname.Len() <= 20)
	{
		PlayerNickname = Nickname;
		UE_LOG(LogTemp, Log, TEXT("Player nickname set to: %s"), *PlayerNickname);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid nickname: %s"), *Nickname);
	}
}

void AERNPlayerState::SetAccountLevel(int32 InLevel)
{
	Server_SetAccountLevel(InLevel);
}

void AERNPlayerState::Server_SetAccountLevel_Implementation(int32 InLevel)
{
	AccountLevel = FMath::Max(1, InLevel);	// 서버에서 세팅 → 자동 복제
}

void AERNPlayerState::Multicast_AddConsumableBuff_Implementation(const FName& ItemID, float Duration)
{
	UE_LOG(LogTemp, Warning, TEXT("[BuffUI] Server Broadcast - Multicast_AddConsumableBuff! ItemID: %s, Duration: %f, PS: %s"), *ItemID.ToString(), Duration, *GetName());
	if (OnConsumableBuffAdded.IsBound())
	{
		OnConsumableBuffAdded.Broadcast(ItemID, Duration);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[BuffUI] OnConsumableBuffAdded delegate is NOT bound on this client! (Is BuffList initialized?)"));
	}
}

void AERNPlayerState::Server_SetReady_Implementation(bool bReady)
{
	// 준비 상태는 로비에서만 변경 허용 — 다른 맵에서의 출발 트리거 방지 (권위 방어)
	const FString CurrentMapName = GetWorld() ? GetWorld()->GetMapName() : FString();
	if (!CurrentMapName.Contains(TEXT("Lobby")))
	{
		return;
	}

	bIsReady = bReady;
	UE_LOG(LogTemp, Log, TEXT("Player %s ready state: %s"), *PlayerNickname, bIsReady ? TEXT("Ready") : TEXT("Not Ready"));

	// 서버에서도 델리게이트 브로드캐스트 (클라이언트는 OnRep_IsReady에서 자동 호출)
	OnReadyStateChanged.Broadcast(bIsReady);

	// GameState에 준비 상태 체크 요청
	if (AERNGameState* GameState = GetWorld()->GetGameState<AERNGameState>())
	{
		GameState->CheckAllPlayersReady();
	}
}

void AERNPlayerState::OnRep_IsReady()
{
	// 준비 상태가 리플리케이트되면 델리게이트 브로드캐스트
	UE_LOG(LogTemp, Log, TEXT("OnRep_IsReady: Player %s is now %s"), *PlayerNickname, bIsReady ? TEXT("Ready") : TEXT("Not Ready"));
	OnReadyStateChanged.Broadcast(bIsReady);
}

void AERNPlayerState::Server_ChangeCharacterClass_Implementation(ECharacterType NewClass)
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("Server_ChangeCharacterClass: PlayerController is null"));
		return;
	}

	// 현재 캐릭터 정보 저장
	APawn* OldPawn = PC->GetPawn();
	FVector Location = OldPawn ? OldPawn->GetActorLocation() : FVector::ZeroVector;
	FRotator Rotation = OldPawn ? OldPawn->GetActorRotation() : FRotator::ZeroRotator;

	// 새 캐릭터 클래스 선택
	TSubclassOf<AProjectERNCharacter> NewCharacterClass = nullptr;
	switch (NewClass)
	{
	case ECharacterType::Warrior:
		NewCharacterClass = WarriorClass;
		break;
	case ECharacterType::Mage:
		NewCharacterClass = MageClass;
		break;
	case ECharacterType::Support:
		NewCharacterClass = SupporterClass;
		break;
	}

	if (!NewCharacterClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Character class not set for type %d"), static_cast<int32>(NewClass));
		return;
	}

	// 새 캐릭터 스폰
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = PC;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// 구 캐릭터 장착 해제 및 인벤토리 정리
	if (AProjectERNCharacter* OldCharacter = Cast<AProjectERNCharacter>(OldPawn))
	{
		if (UERNEquipmentComponent* EquipComp = OldCharacter->FindComponentByClass<UERNEquipmentComponent>())
		{
			// 장착된 무기가 있다면 인벤토리 복사 전에 먼저 해제
			EquipComp->Server_UnequipWeapon();
		}
	}

	AProjectERNCharacter* NewCharacter = GetWorld()->SpawnActor<AProjectERNCharacter>(
		NewCharacterClass, Location, Rotation, SpawnParams);
	if (NewCharacter)
	{
		// 직업 변경 시 기존 무기/소모품 스냅샷 초기화 (새 캐릭터가 구 무기를 덮어쓰는 문제 방지)
		SavedWeaponState = FItemRuntimeState();
		SavedConsumableState = FItemRuntimeState();

		// Possess
		PC->Possess(NewCharacter);

		// CharacterType 업데이트
		CharacterType = NewClass;

		UE_LOG(LogTemp, Warning, TEXT("[Server_ChangeCharacterClass] Player %s changed CharacterType to %d"), *PlayerNickname, static_cast<int32>(NewClass));
	}
	
	if (AProjectERNCharacter* OldCharacter = Cast<AProjectERNCharacter>(OldPawn))
	{
		if (UERNInventoryComponent* OldInventory = OldCharacter->GetInventoryComponent())
		{
			if (UERNInventoryComponent* NewInventory = NewCharacter ? NewCharacter->GetInventoryComponent() : nullptr)
			{
				NewInventory->CopyInventoryFrom(OldInventory);
			}
		}
	}
	
	// 기존 캐릭터 파괴
	if (OldPawn)
	{
		OldPawn->Destroy();
	}
}

void AERNPlayerState::Server_SetCharacterType_Implementation(ECharacterType NewType)
{
	CharacterType = NewType;
	UE_LOG(LogTemp, Log, TEXT("[Server_SetCharacterType] Player %s CharacterType set to %d"), *PlayerNickname, static_cast<int32>(NewType));
}

// AttributeSet에서 값 가져오기
float AERNPlayerState::GetCurrentHealth() const
{
	if (APawn* Pawn = GetPawn())
	{
		if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(Pawn))
		{
			if (const UERNAttributeSet* AS = Character->GetAttributeSet())
			{
				return AS->GetHealth();
			}
		}
	}
	return 0.f;
}

float AERNPlayerState::GetMaxHealth() const
{
	if (APawn* Pawn = GetPawn())
	{
		if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(Pawn))
		{
			if (const UERNAttributeSet* AS = Character->GetAttributeSet())
			{
				return AS->GetMaxHealth();
			}
		}
	}
	return 100.f;
}

float AERNPlayerState::GetCurrentMana() const
{
	if (APawn* Pawn = GetPawn())
	{
		if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(Pawn))
		{
			if (const UERNAttributeSet* AS = Character->GetAttributeSet())
			{
				return AS->GetMana();
			}
		}
	}
	return 0.f;
}

float AERNPlayerState::GetMaxMana() const
{
	if (APawn* Pawn = GetPawn())
	{
		if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(Pawn))
		{
			if (const UERNAttributeSet* AS = Character->GetAttributeSet())
			{
				return AS->GetMaxMana();
			}
		}
	}
	return 100.f;
}

float AERNPlayerState::GetCurrentStamina() const
{
	if (APawn* Pawn = GetPawn())
	{
		if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(Pawn))
		{
			if (const UERNAttributeSet* AS = Character->GetAttributeSet())
			{
				return AS->GetStamina();
			}
		}
	}
	return 0.f;
}

float AERNPlayerState::GetMaxStamina() const
{
	if (APawn* Pawn = GetPawn())
	{
		if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(Pawn))
		{
			if (const UERNAttributeSet* AS = Character->GetAttributeSet())
			{
				return AS->GetMaxStamina();
			}
		}
	}
	return 100.f;
}

float AERNPlayerState::GetCurrentShield() const
{
	if (APawn* Pawn = GetPawn())
	{
		if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(Pawn))
		{
			if (const UERNAttributeSet* AS = Character->GetAttributeSet())
			{
				return AS->GetShield();
			}
		}
	}
	return 0.f;
}

int32 AERNPlayerState::GetCurrentLevel() const
{
	if (APawn* Pawn = GetPawn())
	{
		if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(Pawn))
		{
			if (const UERNAttributeSet* AS = Character->GetAttributeSet())
			{
				return static_cast<int32>(AS->GetLevel());
			}
		}
	}
	return SavedLevel; // 폰이 없으면 스냅샷 값 폴백 (기본 1)
}

int32 AERNPlayerState::GetCurrentFlaskQuantity() const
{
	if (APawn* Pawn = GetPawn())
	{
		if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(Pawn))
		{
			if (const UERNAttributeSet* AS = Character->GetAttributeSet())
			{
				return static_cast<int32>(AS->GetFlaskQuantity());
			}
		}
	}
	return SavedFlaskQuantity;
}

int32 AERNPlayerState::GetMaxFlaskQuantity() const
{
	if (APawn* Pawn = GetPawn())
	{
		if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(Pawn))
		{
			if (const UERNAttributeSet* AS = Character->GetAttributeSet())
			{
				return static_cast<int32>(AS->GetMaxFlaskQuantity());
			}
		}
	}
	return SavedMaxFlaskQuantity;
}

#pragma region Minimap
void AERNPlayerState::SetPlayerNumber_ServerOnly(int32 InNumber)
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	PlayerNumber = InNumber;
}

EERNMinimapIconType AERNPlayerState::GetMinimapPinIconType() const
{
	switch (PlayerNumber)
	{
	case 1:
		return EERNMinimapIconType::PlayerPin1;
	case 2:
		return EERNMinimapIconType::PlayerPin2;
	case 3:
		return EERNMinimapIconType::PlayerPin3;
	default:
		return EERNMinimapIconType::PlayerPin1;
	}
}

EERNMinimapIconType AERNPlayerState::GetMinimapPlayerMarkerIconType() const
{
	switch (PlayerNumber)
	{
	case 1:
		return EERNMinimapIconType::PlayerMarker1;
	case 2:
		return EERNMinimapIconType::PlayerMarker2;
	case 3:
		return EERNMinimapIconType::PlayerMarker3;
	default:
		return EERNMinimapIconType::PlayerMarker1;
	}
}
#pragma endregion

void AERNPlayerState::SaveSnapshotFromPawn()
{
	if (!HasAuthority())
	{
		return;
	}

	AProjectERNCharacter* Char = Cast<AProjectERNCharacter>(GetPawn());
	if (!Char)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Snapshot] SAVE FAIL (%s): no pawn"), *GetPlayerName());
		return;
	}

	if (const UERNAttributeSet* AS = Char->GetAttributeSet())
	{
		SavedLevel = static_cast<int32>(AS->GetLevel());
		SavedGold  = static_cast<int32>(AS->GetGold());
		SavedFlaskQuantity    = static_cast<int32>(AS->GetFlaskQuantity());
		SavedMaxFlaskQuantity = static_cast<int32>(AS->GetMaxFlaskQuantity());
	}
	if (UERNInventoryComponent* Inv = Char->GetInventoryComponent())
	{
		SavedInventory = Inv->GetInventory().GetItems();
	}
	if (UERNEquipmentComponent* Eq = Char->GetEquipmentComponent())
	{
		SavedWeaponState     = Eq->EquipableSlot.GetItemRuntimeState();
		SavedConsumableState = Eq->CurrentConsumable;
	}
	bHasSnapshot = true;

	UE_LOG(LogTemp, Warning, TEXT("[Snapshot] SAVE %s: Level=%d Gold=%d InvItems=%d Weapon=%s"),
		*GetPlayerName(), SavedLevel, SavedGold, SavedInventory.Num(), *SavedWeaponState.GetItemID().ToString());
}

void AERNPlayerState::ClearRunProgress()
{
	if (!HasAuthority())
	{
		return;
	}

	bHasSnapshot = false;
	SavedLevel = 1;
	SavedGold = 0;
	SavedFlaskQuantity = 0;
	SavedMaxFlaskQuantity = 0;
	SavedInventory.Reset();
	SavedWeaponState = FItemRuntimeState();
	SavedConsumableState = FItemRuntimeState();
	KillCount = 0;
	TotalDamageDealt = 0.f;
	TotalLifestealHealed = 0.f;
	TotalUltimateHealed = 0.f;
	TotalBarrierBlocked = 0.f;
}
