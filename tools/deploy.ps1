$ErrorActionPreference = "Stop"

$script_dir = Split-Path -Parent $MyInvocation.MyCommand.Path

$root = Split-Path -Parent $script_dir

$built = Join-Path $root "build\bin\t7-release.dll"

if (-not (Test-Path $built))
{
    Write-Host "deploy: build output not found: $built"

    exit 1
}

function Find-Bo3
{
    $libs = @()

    $steam = $null

    foreach ($key in @("HKCU:\Software\Valve\Steam", "HKLM:\SOFTWARE\WOW6432Node\Valve\Steam", "HKLM:\SOFTWARE\Valve\Steam"))
    {
        try
        {
            $value = (Get-ItemProperty -Path $key -Name SteamPath -ErrorAction Stop).SteamPath

            if ($value)
            {
                $steam = $value -replace "/", "\"

                break
            }
        }
        catch
        {
        }
    }

    if ($steam)
    {
        $libs += $steam

        $vdf = Join-Path $steam "steamapps\libraryfolders.vdf"

        if (Test-Path $vdf)
        {
            foreach ($line in Get-Content $vdf)
            {
                if ($line -match "`"path`"\s+`"([^`"]+)`"")
                {
                    $libs += ($Matches[1] -replace "\\\\", "\")
                }
                elseif ($line -match "^\s*`"\d+`"\s+`"([^`"]+)`"")
                {
                    $libs += ($Matches[1] -replace "\\\\", "\")
                }
            }
        }
    }

    $libs += "C:\Program Files (x86)\Steam"

    foreach ($lib in ($libs | Select-Object -Unique))
    {
        $candidate = Join-Path $lib "steamapps\common\Call of Duty Black Ops III"

        if (Test-Path (Join-Path $candidate "BlackOps3.exe"))
        {
            return $candidate
        }
    }

    return $null
}

$bo3 = Find-Bo3

if (-not $bo3)
{
    Write-Host "deploy: could not locate Black Ops III (BlackOps3.exe not found in any steam library)"

    exit 1
}

$dest = Join-Path $bo3 "d3d11.dll"

try
{
    Copy-Item -Path $built -Destination $dest -Force

    Write-Host "deploy: t7-release.dll -> $dest"
}
catch
{
    Write-Host "deploy: copy failed (game running / dll locked?) - $($_.Exception.Message)"

    exit 1
}
