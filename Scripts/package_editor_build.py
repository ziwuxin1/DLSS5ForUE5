"""Package this plugin's existing Win64 Editor binaries against their engine build.

Example: python Scripts/package_editor_build.py --engine <UE_5.8> --output <build.zip>
Does not build binaries or redistribute the NVIDIA runtime.
"""
import argparse
import json
from pathlib import Path
import subprocess
import zipfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parent.parent
    plugin = repo / "Plugins/DLSS5ForUE5"
    engine = args.engine / "Engine"

    def read_json(path):
        return json.loads(path.read_text(encoding="utf-8-sig"))

    version = read_json(engine / "Build/Build.version")
    modules_path = plugin / "Binaries/Win64/UnrealEditor.modules"
    modules = read_json(modules_path)
    engine_modules = read_json(engine / "Binaries/Win64/UnrealEditor.modules")
    if not modules.get("BuildId") or modules["BuildId"] != engine_modules["BuildId"]:
        parser.error("Plugin and engine module BuildIds differ; rebuild the plugin first.")
    descriptor = read_json(plugin / "DLSS5ForUE5.uplugin")
    if set(modules["Modules"]) != {module["Name"] for module in descriptor["Modules"]}:
        parser.error("Compiled module manifest does not match the plugin descriptor.")
    binaries = []
    for name in modules["Modules"].values():
        if Path(name).name != name or not name.endswith(".dll"):
            parser.error("Expected a local DLL filename in the module manifest.")
        binary = modules_path.parent / name
        if not binary.is_file():
            parser.error("Missing compiled module: " + name)
        binaries.append(binary)

    # PluginManager compares FEngineVersion::CompatibleWith(), not Current().
    # Epic hotfix builds retain patch 0 in that baseline; licensee builds retain
    # their patch number. Keep the actual build version in BUILD.txt instead.
    compatible_patch = version["PatchVersion"] if version["IsLicenseeVersion"] else 0
    descriptor["EngineVersion"] = f'{version["MajorVersion"]}.{version["MinorVersion"]}.{compatible_patch}'
    actual_version = f'{version["MajorVersion"]}.{version["MinorVersion"]}.{version["PatchVersion"]}'
    tracked = subprocess.check_output(
        ["git", "ls-files", "-z", "Plugins/DLSS5ForUE5"], cwd=repo
    ).decode("utf-8").split("\0")
    commit = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=repo).decode().strip()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(args.output, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for relative in filter(None, tracked):
            source = repo / relative
            name = "DLSS5ForUE5/" + source.relative_to(plugin).as_posix()
            if source.name == "DLSS5ForUE5.uplugin":
                archive.writestr(name, json.dumps(descriptor, indent=4) + "\n")
            elif "Binaries" not in source.relative_to(plugin).parts or source.name == "README.md":
                archive.write(source, name)
        for binary in [modules_path, *binaries]:
            archive.write(binary, "DLSS5ForUE5/Binaries/Win64/" + binary.name)
        for relative in ["LICENSE", "THIRD_PARTY_NOTICES.md", "docs/UE584_CAPTURE.md", "docs/VALIDATION_UE584.md"]:
            archive.write(repo / relative, relative)
        archive.write(repo / "Scripts/create_mrq_4k_preset.py", "create_mrq_4k_preset.py")
        archive.writestr("BUILD.txt", f"Source commit: {commit}\nBuilt with UE {actual_version} Win64 Development Editor.\n"
                         f"Compatibility baseline: {descriptor['EngineVersion']}\nModule BuildId: {modules['BuildId']}\n"
                         "NVIDIA runtime excluded; supply your existing nvngx_dlssnr.dll.\n")
    print(f"Packaged UE {actual_version}; compatibility {descriptor['EngineVersion']}; BuildId {modules['BuildId']}")


if __name__ == "__main__":
    main()
