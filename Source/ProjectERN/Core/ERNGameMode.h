// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ERNGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class AERNGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/** Constructor */
	AERNGameMode();

	// 플레이어의 CharacterType에 따라 적절한 Pawn 클래스 반환
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;
	
	// 로그인, 로그아웃에 따른 플레이어 번호 부여
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* ExitingController) override;
	
private:
	int32 AssignPlayerNumber();
	void ReleasePlayerNumber(int32 PlayerNumber);
	
private:
	UPROPERTY()
	TSet<int32> UsedPlayerNumbers;
};
