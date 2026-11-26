"""Run with UE Editor's Tools > Execute Python Script (Python Editor Script Plugin enabled).

Creates a NEW MRQ preset asset. Does not start a render or alter existing presets/jobs.
Requires DLSS5ForUE5 and Movie Render Queue. Set OUTPUT_FORMAT to "exr" for EXR.
"""
import unreal

OUTPUT_FORMAT = "png"
ASSET_DIRECTORY = "/Game/DLSS5"
ASSET_NAME = "DLSS5_4K_Sequence"

if OUTPUT_FORMAT not in ("png", "exr"):
    raise ValueError("OUTPUT_FORMAT must be png or exr")

tools = unreal.AssetToolsHelpers.get_asset_tools()
asset_path, asset_name = tools.create_unique_asset_name(ASSET_DIRECTORY + "/" + ASSET_NAME, "")
preset = tools.create_asset(asset_name, asset_path.rsplit("/", 1)[0],
                            unreal.MoviePipelinePrimaryConfig, unreal.MoviePipelinePrimaryConfigFactory())
if not preset:
    raise RuntimeError("Could not create MRQ preset")

output = preset.find_or_add_setting_by_class(unreal.MoviePipelineOutputSetting)
output.output_resolution = unreal.IntPoint(3840, 2160)
output.output_directory = unreal.DirectoryPath("{project_dir}/Saved/MovieRenders/DLSS5")
output.file_name_format = "{sequence_name}/{shot_name}/{sequence_name}.{frame_number}"
output.zero_pad_frame_numbers = 6
output.override_existing_output = False

preset.find_or_add_setting_by_class(unreal.MoviePipelineDeferredPassBase)
preset.find_or_add_setting_by_class(
    unreal.MoviePipelineImageSequenceOutput_EXR if OUTPUT_FORMAT == "exr"
    else unreal.MoviePipelineImageSequenceOutput_PNG)
aa = preset.find_or_add_setting_by_class(unreal.MoviePipelineAntiAliasingSetting)
aa.spatial_sample_count = 1
aa.temporal_sample_count = 1
aa.engine_warm_up_count = 32
aa.render_warm_up_count = 8
aa.render_warm_up_frames = True
high_res = preset.find_or_add_setting_by_class(unreal.MoviePipelineHighResSetting)
high_res.tile_count = 1
nr = preset.find_or_add_setting_by_class(unreal.MoviePipelineDLSS5Setting)
nr.enable_neural_rendering = True
nr.pass_count = 1
nr.show_watermark = True
unreal.EditorAssetLibrary.save_loaded_asset(preset)
unreal.log("Created DLSS5 4K preset: " + preset.get_path_name())
