// Copyright Epic Games, Inc. All Rights Reserved.

#include "Commandlets/DumpStaticMeshCollisionStateCommandlet.h"

#include "Engine/StaticMesh.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/ConvexElem.h"

namespace
{
	const TCHAR* TargetMeshPath =
		TEXT("/Game/Assets/FC_MedievalMonastery/BackgroundLandscape/SM_Background_Landscape.SM_Background_Landscape");

	FString EnumToString(const UEnum* Enum, int64 Value)
	{
		return Enum ? Enum->GetNameStringByValue(Value) : FString::FromInt(static_cast<int32>(Value));
	}

	FString VectorToString(const FVector3f& Vector)
	{
		return FString::Printf(TEXT("(X=%.3f,Y=%.3f,Z=%.3f)"), Vector.X, Vector.Y, Vector.Z);
	}
}

UDumpStaticMeshCollisionStateCommandlet::UDumpStaticMeshCollisionStateCommandlet()
{
	IsClient = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UDumpStaticMeshCollisionStateCommandlet::Main(const FString& Params)
{
	UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, TargetMeshPath);
	if (!StaticMesh)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load static mesh: %s"), TargetMeshPath);
		return 1;
	}

	UBodySetup* BodySetup = StaticMesh->GetBodySetup();
#if WITH_EDITORONLY_DATA
	const UStaticMesh* ComplexCollisionMesh = StaticMesh->ComplexCollisionMesh;
#endif

	TArray<FString> Lines;
	Lines.Add(TEXT("DumpStaticMeshCollisionStateCommandlet"));
	Lines.Add(FString::Printf(TEXT("AssetPath=%s"), *StaticMesh->GetPathName()));
	Lines.Add(FString::Printf(TEXT("AssetName=%s"), *StaticMesh->GetName()));
	Lines.Add(FString::Printf(TEXT("LOD0Triangles=%d"), StaticMesh->GetNumTriangles(0)));
	Lines.Add(FString::Printf(TEXT("LODForCollision=%d"), StaticMesh->LODForCollision));
#if WITH_EDITORONLY_DATA
	Lines.Add(
		FString::Printf(
			TEXT("ComplexCollisionMesh=%s"),
			ComplexCollisionMesh ? *ComplexCollisionMesh->GetPathName() : TEXT("None")));
	Lines.Add(
		FString::Printf(
			TEXT("ComplexCollisionMeshLOD0Triangles=%d"),
			ComplexCollisionMesh ? ComplexCollisionMesh->GetNumTriangles(0) : -1));
#else
	Lines.Add(TEXT("ComplexCollisionMesh=EditorOnlyDataUnavailable"));
	Lines.Add(TEXT("ComplexCollisionMeshLOD0Triangles=-1"));
#endif

	if (!BodySetup)
	{
		Lines.Add(TEXT("BodySetup=None"));
	}
	else
	{
		const UEnum* TraceEnum = StaticEnum<ECollisionTraceFlag>();
		const UEnum* CollisionEnabledEnum = StaticEnum<ECollisionEnabled::Type>();

		Lines.Add(FString::Printf(TEXT("BodySetup=%s"), *BodySetup->GetPathName()));
		Lines.Add(
			FString::Printf(
				TEXT("CollisionTraceFlag=%s"),
				*EnumToString(TraceEnum, static_cast<int64>(BodySetup->CollisionTraceFlag))));
		Lines.Add(
			FString::Printf(
				TEXT("DefaultCollisionEnabled=%s"),
				*EnumToString(
					CollisionEnabledEnum,
					static_cast<int64>(BodySetup->DefaultInstance.GetCollisionEnabled()))));
		Lines.Add(
			FString::Printf(
				TEXT("SimpleCollisionCounts Boxes=%d Spheres=%d Capsules=%d Convex=%d TaperedCapsules=%d"),
				BodySetup->AggGeom.BoxElems.Num(),
				BodySetup->AggGeom.SphereElems.Num(),
				BodySetup->AggGeom.SphylElems.Num(),
				BodySetup->AggGeom.ConvexElems.Num(),
				BodySetup->AggGeom.TaperedCapsuleElems.Num()));

		if (BodySetup->AggGeom.ConvexElems.Num() > 0)
		{
			const FKConvexElem& Convex = BodySetup->AggGeom.ConvexElems[0];
			FVector3f Min(FLT_MAX, FLT_MAX, FLT_MAX);
			FVector3f Max(-FLT_MAX, -FLT_MAX, -FLT_MAX);
			FVector3f Sum(0.0f, 0.0f, 0.0f);

			for (const FVector& Vertex : Convex.VertexData)
			{
				const FVector3f Vertex3f(Vertex);
				Min.X = FMath::Min(Min.X, Vertex3f.X);
				Min.Y = FMath::Min(Min.Y, Vertex3f.Y);
				Min.Z = FMath::Min(Min.Z, Vertex3f.Z);
				Max.X = FMath::Max(Max.X, Vertex3f.X);
				Max.Y = FMath::Max(Max.Y, Vertex3f.Y);
				Max.Z = FMath::Max(Max.Z, Vertex3f.Z);
				Sum += Vertex3f;
			}

			Lines.Add(FString::Printf(TEXT("Convex0VertexCount=%d"), Convex.VertexData.Num()));
			Lines.Add(FString::Printf(TEXT("Convex0Sum=%s"), *VectorToString(Sum)));
			Lines.Add(FString::Printf(TEXT("Convex0Min=%s"), *VectorToString(Min)));
			Lines.Add(FString::Printf(TEXT("Convex0Max=%s"), *VectorToString(Max)));
		}
	}

	const FString OutputPath =
		FPaths::ProjectSavedDir() / TEXT("CurrentStaticMeshCollisionState.txt");
	if (!FFileHelper::SaveStringArrayToFile(Lines, *OutputPath))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to save report to %s"), *OutputPath);
		return 1;
	}

	for (const FString& Line : Lines)
	{
		UE_LOG(LogTemp, Display, TEXT("%s"), *Line);
	}

	UE_LOG(LogTemp, Display, TEXT("Saved report to %s"), *OutputPath);
	return 0;
}
