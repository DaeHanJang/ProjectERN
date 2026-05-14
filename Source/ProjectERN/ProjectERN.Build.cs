// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectERN : ModuleRules
{
	public ProjectERN(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"SlateCore",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"Niagara",
			"PhysicsCore",
			"MotionWarping",
			"NetCore",
			"LevelSequence",
			"MovieScene"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ProjectERN",
			"ProjectERN/Variant_Platforming",
			"ProjectERN/Variant_Platforming/Animation",
			"ProjectERN/Variant_Combat",
			"ProjectERN/Variant_Combat/AI",
			"ProjectERN/Variant_Combat/Animation",
			"ProjectERN/Variant_Combat/Gameplay",
			"ProjectERN/Variant_Combat/Interfaces",
			"ProjectERN/Variant_Combat/UI",
			"ProjectERN/Variant_SideScrolling",
			"ProjectERN/Variant_SideScrolling/AI",
			"ProjectERN/Variant_SideScrolling/Gameplay",
			"ProjectERN/Variant_SideScrolling/Interfaces",
			"ProjectERN/Variant_SideScrolling/UI",
			
			// Shop System
			"ProjectERN/Shop",
			"ProjectERN/Shop/Data",
			"ProjectERN/Shop/Provider",
			"ProjectERN/Shop/Components",
			"ProjectERN/Shop/Actors"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
