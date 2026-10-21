"""Checks Asylum's real x86 call sites/stack cleanup against the adapter ABI.
This is STATIC evidence, never a claim about render re-entry in a running game.
"""
from pathlib import Path
import hashlib,struct,re
root=Path(__file__).resolve().parents[1]
exe=Path(r'E:/Games/Steam/steamapps/common/Batman Arkham Asylum GOTY/Binaries/ShippingPC-BmGame.exe')
raw=exe.read_bytes()
assert hashlib.sha256(raw).hexdigest() in {'4dac1f5e2ac6710b7378fdce74601f616f4753e3756cb5fda63c7519cc2eb028', # Steam GOTY
 'ed65ce3c52baca7adaea00cb0e5def3d8a78bd05f7ceacd68a68462e92d8dab2'} # + Joker DLC Patcher (2 bytes in 0x16153C0)
pe=struct.unpack_from('<I',raw,0x3c)[0];sections=struct.unpack_from('<H',raw,pe+6)[0]
optional=struct.unpack_from('<H',raw,pe+20)[0];base=struct.unpack_from('<I',raw,pe+24+28)[0]
def at(va,n):
 rva=va-base
 for i in range(sections):
  s=pe+24+optional+40*i;size,vaddr,rsize,offset=struct.unpack_from('<IIII',raw,s+8)
  if vaddr<=rva<vaddr+size:return raw[offset+rva-vaddr:offset+rva-vaddr+n]
 raise AssertionError(hex(va))
source=(root/'src/game/asylum/native_stereo.cpp').read_text()
for rva,sig in re.findall(r'\{(0x[0-9a-f]+),"([0-9a-f]{32})"',source):assert at(base+int(rva,16),16)==bytes.fromhex(sig)
assert at(0xccb472,1)==b'\xc3' # Draw: no stack arguments
assert at(0x89f090,3)==bytes.fromhex('c21400') # Calc: five words
assert at(0xaed7d2,3)==bytes.fromhex('c26000') # ctor: 24 words
assert at(0x6d1f31,3)==bytes.fromhex('c20400') # movie tick: one float
assert at(0x6d2289,3)==bytes.fromhex('c20c00') # movie stop: three words
assert at(0x6cca54,3)==bytes.fromhex('c20400') # render-thread force-stop: one word
assert at(0x6cca00,16)==bytes.fromhex('5633c0578b7c240c3bf8be0100000089') # runtime render-stop entry guard
assert at(0x6cca4b,5)==bytes.fromhex('e8b0e7ffff') # force-stop calls native cleanup on owning render thread
assert at(0x6cb2b0,6)==bytes.fromhex('397c240c740b') # cleanup advances startup list only when its argument is true
assert at(0x6cb2e6,3)==bytes.fromhex('c20400') # native cleanup: one word
assert at(0x89f012,5)==bytes.fromhex('e849dc2400') # main-view ctor return -> 89f017
assert at(0xaeccaa,3)==bytes.fromhex('8b7540') # view pointer: stack word 15
assert at(0xaecd07,3)==bytes.fromhex('8b7544') # projection pointer: word 16
assert at(0x89f868,5)==bytes.fromhex('e8332d2500') # local player uses this allocator
assert at(0x1ecff08+0x1c,4)==struct.pack('<I',0x6d1fa0) # movie interface stop slot
assert at(0x23a5f88+8,4)==struct.pack('<I',0x8a0230) # LIVE client's draw entry
assert at(0xab0092,3)==bytes.fromhex('c20800') # canvas ctor: two stack words
assert at(0xab0054,5)==bytes.fromhex('e8177fb7ff') # canvas transform hash is this target's CRC
assert at(0x627fa8,1)==b'\xc3' # CRC: cdecl, caller cleans three words
assert at(0xab005c,3)==bytes.fromhex('894640') # transform hash stored after its 64B matrix
assert at(0xaaffaa,3)==bytes.fromhex('8d7b24') # FCanvas transform array at +24
assert at(0x9b62d9,12)==bytes.fromhex('8bb190020000c1ee0a83e601') # IsMenuLevel cached flag +290 bit10
assert at(0x9cb716,14)==bytes.fromhex('c1e00a3381900200002500040000') # map-change setter mask400
assert at(0x9c6fb1,14)==bytes.fromhex('a1b8626b028b70448b463483c630') # GEngine/client/viewports read chain
assert at(0x25e0a48,8)==struct.pack('<II',0x2065cc4,0xa588d0) # native Canvas.Project registration
assert at(0xa58942,5)==bytes.fromhex('e859390600') # execProject -> ABC2A0, return A58947
assert at(0xabc2a0,16)==bytes.fromhex('558bec83e4f08b450cf30f107008f30f') # exact projection hook guard
assert at(0xabc3b8,3)==bytes.fromhex('c20800') # project: out4 + world3, two stack words
assert at(0xabc2ea,8)==bytes.fromhex('f30f108910020000') # actual view VP+210
assert at(0x158a366,5)==bytes.fromhex('e8351f53ff') # exact detective actor callout -> Project
assert at(0x158a36b,8)==bytes.fromhex('8b088b50048b4008') # consume original XY/Z
assert at(0x158a3a6,8)==bytes.fromhex('f30f1054247cf30f') # native normalized XY consumption
assert at(0x158a42e,5)==bytes.fromhex('e8cd85fdff') # actor callout record append
assert at(0x1562a58,3)==bytes.fromhex('c20400') # original 32-byte record copy, one word
assert at(0x1e3f070,4)==struct.pack('<f',.5) # target normalized coordinates
assert at(0x25e0a38,8)==struct.pack('<II',0x2065d3c,0xa58ac0) # native WorldToPixel registry
assert at(0xa58b6e,5)==bytes.fromhex('e84dab0500') # separate pixel helper caller
assert at(0xab36c0,16)==bytes.fromhex('558bec83e4f083ec108b4508f30f1060') # exact hook entry
assert at(0xab37e6,3)==bytes.fromhex('c20800') # world3/out2 stack, ret8
assert at(0xab37ee,3)==bytes.fromhex('c20800') # native failure also ret8
assert at(0xab37a5,5)==bytes.fromhex('f30f594130') # actual view SizeX float+30
assert at(0xab37bb,5)==bytes.fromhex('f30f595134') # actual view SizeY float+34
assert at(0xab37de,5)==bytes.fromhex('b801000000') # success1; failure branch0
print('PASS STATIC: target hash, loaded instruction guards, Draw/Calc/ctor 0/5/24-word ABIs, view/projection arguments, own view-state allocator and game/render-thread movie-stop ABIs including force cleanup without startup advancement.')
