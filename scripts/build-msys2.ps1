# Build OS-dev using MSYS2 bash (Windows)
# Prerequisites:
#   1. MSYS2 installed (e.g. C:\msys64)
#   2. In MSYS2: pacman -S nasm make xorriso
#   3. x86_64-elf toolchain and grub-mkrescue in PATH, or set TOOLCHAIN_DIR and GRUB_DIR below

param(
    [string]$Msys2Path = "C:\msys64",
    [string]$ToolchainDir = "",
    [string]$GrubDir = ""
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path $ProjectRoot)) {
    $ProjectRoot = Get-Location
}

$bash = Join-Path $Msys2Path "usr\bin\bash.exe"
if (-not (Test-Path $bash)) {
    Write-Host "MSYS2 bash not found at: $bash"
    Write-Host "Install MSYS2 from https://www.msys2.org or set -Msys2Path"
    exit 1
}

$pathAdd = ""
if ($ToolchainDir -and (Test-Path $ToolchainDir)) {
    $pathAdd = "$ToolchainDir;$pathAdd"
}
if ($GrubDir -and (Test-Path $GrubDir)) {
    $pathAdd = "$GrubDir;$pathAdd"
}
$pathEnv = if ($pathAdd) { "export PATH='$($pathAdd -replace ';',"'\":\"'):`$PATH'; " } else { "" }

$projectPath = $ProjectRoot -replace '\\', '/'
$drive = $projectPath.Substring(0, 1).ToLower()
$unixPath = "/$drive/" + ($projectPath.Substring(3) -replace '\\', '/')

$cmd = "cd '$unixPath' && $pathEnv make"
Write-Host "[*] Running: bash -lc `"$cmd`""
& $bash -lc $cmd
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed. Ensure x86_64-elf-gcc and grub-mkrescue are in PATH (or set -ToolchainDir and -GrubDir)."
    exit $LASTEXITCODE
}
Write-Host "[+] Done: $ProjectRoot\build\os.iso"
