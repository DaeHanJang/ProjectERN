#pragma once

// 레벨에 배치할 건물 종류를 StructureSpawnPoint와 StructureSpawnConfigDataAsset에서 서로 비교하는 데 사용
UENUM(BlueprintType)
enum class EStructureType : uint8
{
	None UMETA(DisplayName = "None"),
	Castle UMETA(DisplayName = "Castle"),
	Church UMETA(DisplayName = "Church")
};
