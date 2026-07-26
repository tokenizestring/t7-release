param(
    [string]$tag = "latest",
    [string]$title = "Latest build"
)

$ErrorActionPreference = "Stop"

$gh = "C:\Program Files\GitHub CLI\gh.exe"

if (-not (Test-Path $gh)) {
    $gh = "gh"
}

$repo = "tokenizestring/t7-release"

$src = Join-Path $PSScriptRoot "..\build\bin\t7-release.dll"

if (-not (Test-Path $src)) {
    Write-Error "build\bin\t7-release.dll not found - run build.bat first"
    exit 1
}

$asset = Join-Path $env:TEMP "d3d11.dll"

Copy-Item $src $asset -Force

$notes = "Automated build. Drop d3d11.dll next to BlackOps3.exe (it proxies the real d3d11)."

if ($tag -eq "latest") {
    & $gh release delete $tag -R $repo --yes --cleanup-tag 2>$null
    & $gh release create $tag $asset -R $repo --target main --title $title --notes $notes
} else {
    & $gh release create $tag $asset -R $repo --target main --title $title --notes $notes
}

if ($LASTEXITCODE -eq 0) {
    Write-Host "released $tag with d3d11.dll -> https://github.com/$repo/releases/tag/$tag"
} else {
    Write-Error "release failed (is gh authenticated? run: gh auth login)"
}
