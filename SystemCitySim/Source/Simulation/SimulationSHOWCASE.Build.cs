

using UnrealBuildTool;

public class SimulationSHOWCASE : ModuleRules
{
	public SimulationSHOWCASE(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayTags", "AIModule", "NavigationSystem" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		
		
		
		
		

		
	}
}
