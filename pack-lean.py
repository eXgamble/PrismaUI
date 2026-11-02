"""Copy the built dist/ into the MO2 mod "Prisma UI - CEF lean", minus what a game HUD never loads.

python pack-lean.py [mod folder]
"""
import pathlib
import shutil
import sys

ROOT = pathlib.Path(__file__).parent
DIST = ROOT / "dist" / "PrismaUI_2.0.0"
MOD = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else r"C:\Modding\MO2\mods\Prisma UI - CEF lean")

# Left out (paths under dist/, lower case). Each was checked in game: the page still loads and draws.
SKIP_FILES = {
    "prismaui/libs/prismauicefsubprocess.pdb",  # debug symbols
    "skse/plugins/prismaui.pdb",
    "prismaui/libs/chrome_200_percent.pak",     # Chromium's own UI art at 200% (it shows no UI of its own)
    "prismaui/libs/dxcompiler.dll",              # DirectX 12 shader compiler (WebGPU); ANGLE runs on D3D11
    "prismaui/libs/dxil.dll",
    "prismaui/libs/vk_swiftshader.dll",          # software GPU, only used when the real GPU can't be
    "prismaui/libs/vk_swiftshader_icd.json",
    "prismaui/libs/vulkan-1.dll",
}
KEEP_LOCALES = ("en-us",)  # CEF needs the locale it starts with (settings.locale = en-US)


def wanted(rel: str) -> bool:
    low = rel.lower()
    if low in SKIP_FILES:
        return False
    if low.startswith("prismaui/libs/locales/"):
        return pathlib.PurePosixPath(low).name.startswith(KEEP_LOCALES)
    return True


def main():
    if not DIST.is_dir():
        raise SystemExit(f"no build at {DIST}: run build-local.bat first")
    keep_meta = (MOD / "meta.ini").read_bytes() if (MOD / "meta.ini").exists() else None
    keep_lean = (MOD / "PrismaUI" / "lean.txt").read_bytes() if (MOD / "PrismaUI" / "lean.txt").exists() else None
    if MOD.exists():
        shutil.rmtree(MOD)
    total = 0
    for src in DIST.rglob("*"):
        if not src.is_file():
            continue
        rel = src.relative_to(DIST).as_posix()
        if rel == "NOTICES.txt" or wanted(rel):
            dst = MOD / rel
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, dst)
            total += src.stat().st_size
    if keep_meta is not None:
        (MOD / "meta.ini").write_bytes(keep_meta)
    lean = MOD / "PrismaUI" / "lean.txt"
    lean.write_bytes(keep_lean if keep_lean is not None else (ROOT / "lean.txt").read_bytes())
    print(f"{MOD}: {total / 1e6:.0f} MB")


main()
