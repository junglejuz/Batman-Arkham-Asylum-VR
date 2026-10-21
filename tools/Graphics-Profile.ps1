function Get-AsylumConfigPaths([string]$GameDir) {
    # UE3 can rebuild BmEngine from DefaultEngine during startup. Cover both
    # ends of that merge; the journal restores every byte after the game exits.
    $paths=@((Join-Path $GameDir 'BmGame/Config/DefaultEngine.ini'),(Join-Path $GameDir 'BmGame/Config/BmEngine.ini'))
    $docs=[Environment]::GetFolderPath('MyDocuments')
    foreach($relative in @('Square Enix/Batman Arkham Asylum GOTY','Eidos/Batman Arkham Asylum')) {
        foreach($name in @('BmEngine.ini','UserEngine.ini')) {
            $p=Join-Path $docs ($relative+'/BmGame/Config/'+$name)
            if(Test-Path -LiteralPath $p) { $paths+=$p }
        }
    }
    return $paths
}
function Set-AsylumVRProfile([string]$Path,[int]$Width,[int]$Height) {
    $bytes=[IO.File]::ReadAllBytes($Path)
    # Preserve bytes/BOM/line endings. Retail Asylum configs here are ANSI;
    # Latin-1 is a reversible byte mapping for changes to ASCII keys/values.
    $encoding=[Text.Encoding]::GetEncoding(28591)
    if($bytes.Length -ge 2 -and $bytes[0] -eq 255 -and $bytes[1] -eq 254) {$encoding=[Text.Encoding]::Unicode}
    elseif($bytes.Length -ge 3 -and $bytes[0] -eq 239 -and $bytes[1] -eq 187 -and $bytes[2] -eq 191) {$encoding=[Text.Encoding]::UTF8}
    $text=$encoding.GetString($bytes)
    $blocks=[regex]::Matches($text,'(?ms)(^\[SystemSettings\]\r?\n)(.*?)(?=^\[|\z)')
    if($blocks.Count -ne 1) {throw "SystemSettings section ambiguous: $Path"}
    $block=$blocks[0];$body=$block.Groups[2].Value
    $newline=if($body.Contains("`r`n")){"`r`n"}else{"`n"}
    foreach($option in @(@('Fullscreen','False'),@('UseVsync','False'),@('ResX',"$Width"),@('ResY',"$Height"),@('MotionBlur','False'),@('DepthOfField','False'),@('MaxMultisamples','1'))) {
        $key=$option[0];$value=$option[1];$pattern='(?mi)^'+$key+'=[^\r\n]*'
        if([regex]::Matches($body,$pattern).Count -gt 1) {throw "Duplicate $key in $Path"}
        if([regex]::IsMatch($body,$pattern)) {$body=[regex]::Replace($body,$pattern,{param($m) "$key=$value"})}
        else {$body+="$key=$value$newline"}
    }
    $text=$text.Substring(0,$block.Index)+$block.Groups[1].Value+$body+$text.Substring($block.Index+$block.Length)
    $text=[regex]::Replace($text,'(?mi)^bSmoothFrameRate=[^\r\n]*','bSmoothFrameRate=False')
    # Fallback ceiling if the game re-enables smoothing. XR owns the actual
    # cadence; this ceiling must not throttle a 90/120Hz headset back to62.
    $text=[regex]::Replace($text,'(?mi)^MaxSmoothedFrameRate=[^\r\n]*','MaxSmoothedFrameRate=300')
    [IO.File]::WriteAllBytes($Path,$encoding.GetBytes($text))
}
function Get-AsylumProfileStatus([string[]]$Paths) {
    foreach($path in $Paths) {
        $text=[IO.File]::ReadAllText($path)
        $values=[ordered]@{}
        foreach($key in @('bSmoothFrameRate','MaxSmoothedFrameRate','UseVsync','DepthOfField','MotionBlur','ResX','ResY')) {
            $values[$key]=@([regex]::Matches($text,'(?mi)^'+$key+'=([^\r\n]*)') | ForEach-Object {$_.Groups[1].Value})
        }
        @{path=$path;sha256=(Get-FileHash -LiteralPath $path).Hash;values=$values}
    }
}
function Test-AsylumConfigWritable([string]$Path) {
    # Opens for writing without changing a byte; returns why Windows refused, or nothing. Catches the Read-only
    # attribute, NTFS permissions and Windows Security's Controlled folder access (blocks PowerShell in Documents).
    try {[IO.File]::Open($Path,'Open','Write','ReadWrite').Dispose()}
    catch {return $_.Exception.GetBaseException().Message}
}
function Select-AsylumWritableConfigs([string]$GameDir,[string[]]$Paths) {
    # Player report 2026-10-20 (WRITEDENIED): a Documents copy Windows will not let us write is left as it is and
    # the launch goes on (the command line still sets window and size). The game folder's own two files hold the
    # frame cap, so those must be writable; nothing has been changed when this stops.
    $root=[IO.Path]::GetFullPath($GameDir).TrimEnd('\','/')+'\'
    foreach($path in $Paths) {
        $why=Test-AsylumConfigWritable $path
        if(!$why) {$path;continue}
        if([IO.Path]::GetFullPath($path).StartsWith($root,[StringComparison]::OrdinalIgnoreCase)) {
            throw "Windows does not allow changes to the game's settings file $path ($why). Untick Read-only in its Properties, or allow PowerShell in Windows Security > Virus & threat protection > Ransomware protection > Controlled folder access, then start VR again. Nothing was changed."
        }
        Write-Warning "Windows does not allow changes to $path ($why). It is left as it is and VR starts anyway; if the game runs capped or fullscreen, untick Read-only on that file or allow PowerShell in Windows Security > Ransomware protection > Controlled folder access."
    }
}
function Restore-AsylumProfile($Profile) {
    # A file already equal to its original is left alone: a write Windows refused at launch must not also fail
    # here, or the journal stays and every later launch, install and uninstall stops on it (WRITEDENIED).
    # One file that cannot be put back does not stop the others; the error at the end names each one.
    $failed=@()
    foreach($record in $Profile.files) {
        if((Test-Path -LiteralPath $record.path) -and (Get-FileHash -LiteralPath $record.path).Hash -eq $record.sha256) {continue}
        if((Get-FileHash -LiteralPath $record.backup).Hash -ne $record.sha256) {throw "Profile backup changed: $($record.backup)"}
        try {[IO.File]::WriteAllBytes($record.path,[IO.File]::ReadAllBytes($record.backup))}
        catch {$failed+="$($record.path) ($($_.Exception.GetBaseException().Message)); your original is saved at $($record.backup)";continue}
        if((Get-FileHash -LiteralPath $record.path).Hash -ne $record.sha256) {throw "Profile restore failed: $($record.path)"}
    }
    if($failed.Count) {throw "Windows did not allow your original graphics settings to be put back: $($failed -join '; '). Copy each original back by hand, or allow PowerShell in Windows Security > Ransomware protection > Controlled folder access, then start VR again."}
}
function Remove-AsylumProfileBackups([string]$DataDir,[string]$Keep) {
    # Player report 2026-10-21 (PROFILECLEAN): every launch left its profiles/<time> folder behind. Call only once the
    # originals are back and the journal is gone; a failed restore throws first, so its journal and backups stay.
    # Also sweeps folders earlier builds left. Only launcher-made timestamp folders are touched, never $Keep.
    $root=Join-Path $DataDir 'profiles'
    if(!(Test-Path -LiteralPath $root)) {return}
    foreach($dir in @(Get-ChildItem -LiteralPath $root -Directory | Where-Object Name -Match '^\d{8}_\d{6}_\d{3}$')) {
        if($Keep -and [IO.Path]::GetFullPath($dir.FullName) -eq [IO.Path]::GetFullPath($Keep)) {continue}
        try {Remove-Item -LiteralPath $dir.FullName -Recurse -Force} catch {Write-Warning "Could not delete the old settings backup $($dir.FullName): $_"}
    }
}
