// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "World/Data/ERNMinimapIconTypes.h"
#include "ERNPlayerState.generated.h"

UENUM(BlueprintType)
enum class ECharacterType : uint8
{
	None,
	Warrior,
	Support,
	Mage
};

// 준비 상태 변경 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReadyStateChanged, bool, bIsReady);

UCLASS()
class PROJECTERN_API AERNPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AERNPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 캐릭터 타입
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Character")
	ECharacterType CharacterType;

	// 플레이어 닉네임
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Player Info")
	FString PlayerNickname;

	// 준비 상태 (로비용)
	UPROPERTY(ReplicatedUsing=OnRep_IsReady, BlueprintReadOnly, Category = "Player Info")
	bool bIsReady;

	// 준비 상태 변경 이벤트 (블루프린트에서 바인딩 가능)
	UPROPERTY(BlueprintAssignable, Category = "Player Info")
	FOnReadyStateChanged OnReadyStateChanged;

	// 블루프린트에서 닉네임 설정 (자동으로 서버 RPC 호출)
	UFUNCTION(BlueprintCallable, Category = "Player Info")
	void SetNickname(const FString& Nickname);

protected:
	// bIsReady 리플리케이션 콜백
	UFUNCTION()
	void OnRep_IsReady();

public:

	// 서버에 닉네임 설정 요청
	UFUNCTION(Server, Reliable)
	void Server_SetNickname(const FString& Nickname);

	// 서버에 준비 상태 설정 요청
	UFUNCTION(Server, Reliable)
	void Server_SetReady(bool bReady);

	// 서버에 캐릭터 클래스 변경 요청
	UFUNCTION(Server, Reliable)
	void Server_ChangeCharacterClass(ECharacterType NewClass);

	// 서버에 캐릭터 타입 설정 요청 (맵 이동 시 복원용)
	UFUNCTION(Server, Reliable)
	void Server_SetCharacterType(ECharacterType NewType);

	// 각 클래스별 캐릭터 블루프린트
	UPROPERTY(EditDefaultsOnly, Category = "Character")
	TSubclassOf<class AProjectERNCharacter> WarriorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Character")
	TSubclassOf<class AProjectERNCharacter> MageClass;

	UPROPERTY(EditDefaultsOnly, Category = "Character")
	TSubclassOf<class AProjectERNCharacter> SupporterClass;

	// AttributeSet에서 HP/MP/Stamina 가져오기 (블루프린트용 헬퍼)
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Attributes")
	float GetCurrentHealth() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Attributes")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Attributes")
	float GetCurrentMana() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Attributes")
	float GetMaxMana() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Attributes")
	float GetCurrentStamina() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Attributes")
	float GetMaxStamina() const;
	
	// 플레이어 접속 번호
#pragma region Minimap
public:
	//접속 순서에 따른 플레이어 넘버. 만약 직업이 겹쳐도 구분 가능
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Player Info")
	int32 PlayerNumber = 0;
	
	void SetPlayerNumber_ServerOnly(int32 InNumber);
	
	EERNMinimapIconType GetMinimapPinIconType() const;
	EERNMinimapIconType GetMinimapPlayerMarkerIconType() const;
#pragma endregion Minimap
};
