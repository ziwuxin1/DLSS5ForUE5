using UnrealBuildTool;

public class DLSS5ForUE5MoviePipeline : ModuleRules
{
    public DLSS5ForUE5MoviePipeline(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "MovieRenderPipelineCore" });
        PrivateDependencyModuleNames.AddRange(new[] { "DLSS5ForUE5_nvngx", "MovieRenderPipelineRenderPasses" });
    }
}
