# Batman Asylum VR release package (PACKAGING_GUIDE.md, made like batman_origins/tools/make_package.py): builds
# dist/BatmanAsylumVR-ArkhamAsylum.zip from one source of truth per file - the mod from build-native9/Release, the DLSS
# helper and NVIDIA's DLSS library from build64/Release, the launcher scripts from tools/, the installer from install/,
# JJ's tested VR settings from his live aa_rendering.ini. Zip made with zipfile: forward slashes, no top folder
# (Windows' "Extract All" rejects backslash entries). NVIDIA's DLSS library ships in the release zip only - never
# committed to the repository.
#   python tools/make_package.py [--settings "<aa_rendering.ini>"]
import argparse, hashlib, os, re, shutil, sys, zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
VERSION = '0.1.5'
ap = argparse.ArgumentParser()
ap.add_argument('--settings', default=os.path.join(os.environ.get('LOCALAPPDATA', ''), 'Batman_Asylum', 'aa_rendering.ini'))
a = ap.parse_args()

dist = os.path.join(ROOT, 'dist')
stage = os.path.join(dist, 'BatmanAsylumVR')
zpath = os.path.join(dist, 'BatmanAsylumVR-ArkhamAsylum.zip')
if os.path.isdir(stage): shutil.rmtree(stage)
os.makedirs(os.path.join(stage, 'files'))

def put(src, rel):
    dst = os.path.join(stage, rel)
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    if os.path.isdir(src): shutil.copytree(src, dst)
    else: shutil.copy2(src, dst)

inst = os.path.join(ROOT, 'install')
for f in ('Install-BatmanAsylumVR.bat', 'Install-BatmanAsylumVR.ps1', 'Uninstall-BatmanAsylumVR.bat',
          'Uninstall-BatmanAsylumVR.ps1', 'Batman Asylum VR.bat', 'Batman Asylum VR (SteamVR).bat'):
    put(os.path.join(inst, f), f)
rel = os.path.join(ROOT, 'build-native9', 'Release')
for f in ('d3d9.dll', 'xinput1_3.dll', 'batman_asylum.dll', 'aa_xr_config32.exe'):
    put(os.path.join(rel, f), f)
av = os.path.join('files', 'AsylumVR')
for f in ('Launch-VR.ps1', 'Graphics-Profile.ps1', 'Render-Size.ps1'):
    put(os.path.join(HERE, f), os.path.join(av, f))
for src, name in ((os.path.join(ROOT, 'LICENSE-bioshock-vr.txt'), 'LICENSE-bioshock-vr.txt'),
                  (os.path.join(ROOT, 'LICENSE-luma-framework.md'), 'LICENSE-luma-framework.md'),
                  (os.path.join(ROOT, 'LICENSE-NVAPI.txt'), 'LICENSE-NVAPI.txt'),
                  (os.path.join(ROOT, 'third_party', 'minhook', 'LICENSE.txt'), 'LICENSE-MinHook.txt'),
                  (os.path.join(ROOT, 'third_party', 'imgui', 'LICENSE.txt'), 'LICENSE-ImGui.txt'),
                  (os.path.join(ROOT, 'third_party', 'OpenXR-SDK', 'LICENSE'), 'LICENSE-OpenXR.txt')):
    put(src, os.path.join(av, name))
b64 = os.path.join(ROOT, 'build64', 'Release')
put(os.path.join(b64, 'aa_dlss64.exe'), os.path.join(av, 'DLSS', 'aa_dlss64.exe'))
put(os.path.join(b64, 'nvngx_dlss.dll'), os.path.join(av, 'DLSS', 'nvngx_dlss.dll'))
put(a.settings, os.path.join('files', 'aa_rendering.ini'))
# FIRSTPERSON: new players start in third person whatever JJ last used (City SHIP_OVERRIDES first_person=0).
sp = os.path.join(stage, 'files', 'aa_rendering.ini')
st = open(sp, encoding='latin-1').read()
open(sp, 'w', encoding='latin-1', newline='').write(re.sub(r'(?im)^FirstPerson=1', 'FirstPerson=0', st))
open(os.path.join(stage, 'files', 'version.txt'), 'w', newline='\n').write(VERSION + '\n')
for f in ('README.md', 'LICENSE-bioshock-vr.txt', 'LICENSE-luma-framework.md', 'LICENSE-NVAPI.txt', 'THIRD_PARTY_NOTICES.md'):
    put(os.path.join(ROOT, f), f)

# checks: batch files CRLF, both proxies are ours, the settings file is a tested one (no test switches), the README is
# the player one, the DLL is a release build
bad = []
for dp, _, fs in os.walk(stage):
    for f in fs:
        if f.lower().endswith('.bat'):
            b = open(os.path.join(dp, f), 'rb').read()
            if b.count(b'\n') != b.count(b'\r\n'): bad.append(f + ': not CRLF')
for f in ('d3d9.dll', 'xinput1_3.dll'):
    if b'[batman_asylum-proxy]' not in open(os.path.join(stage, f), 'rb').read(): bad.append(f + ' is not ours')
if b'Asylum-0.1.5' not in open(os.path.join(stage, 'batman_asylum.dll'), 'rb').read(): bad.append('batman_asylum.dll is not the 0.1.5 build')
st = open(os.path.join(stage, 'files', 'aa_rendering.ini'), encoding='latin-1').read()
if '[AsylumVR]' not in st: bad.append('aa_rendering.ini lacks [AsylumVR]')
if re.search(r'(?im)^(AutoCaptures|HideTargetlessGrapple|FixStaleParticles)\s*=\s*1', st): bad.append('aa_rendering.ini has a test switch on')
if 'CURRENT A2' in open(os.path.join(stage, 'README.md'), encoding='utf-8').read(): bad.append('README is the development notes one')
if bad:
    print('REFUSED:', *bad, sep='\n  '); sys.exit(1)

h = lambda p: hashlib.sha256(open(p, 'rb').read()).hexdigest()
if os.path.exists(zpath): os.remove(zpath)
with zipfile.ZipFile(zpath, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
    for dp, _, fs in os.walk(stage):
        for f in sorted(fs):
            full = os.path.join(dp, f)
            arc = os.path.relpath(full, stage).replace(os.sep, '/')
            z.write(full, arc, zipfile.ZIP_STORED if f.lower().endswith(('.dll', '.exe')) else zipfile.ZIP_DEFLATED)
with zipfile.ZipFile(zpath) as z:
    assert z.testzip() is None
    names = z.namelist()
    assert not any('\\' in n for n in names), 'backslash in an entry name'
for n in names: print('  ', n)
print(f'{zpath}: {len(names)} files, {os.path.getsize(zpath) / 1e6:.1f} MB, sha256 {h(zpath)}')
