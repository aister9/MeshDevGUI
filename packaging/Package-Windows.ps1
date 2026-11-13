param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^\d+\.\d+\.\d+([-.][A-Za-z0-9.]+)?$')]
    [string]$Version,
    [string]$BuildDirectory = 'build-vs2026-cuda135',
    [string]$CudaDirectory = 'C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.5'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
$build = (Resolve-Path (Join-Path $root $BuildDirectory)).Path
$cache = @{}
Get-Content (Join-Path $build 'CMakeCache.txt') | ForEach-Object {
    if ($_ -match '^([^#/][^:]*):[^=]+=(.*)$') { $cache[$matches[1]] = $matches[2] }
}
$vs = $cache['CMAKE_GENERATOR_INSTANCE']
if (!$vs) { throw 'A configured Visual Studio build is required.' }
$toolVersion = (Get-Content (Join-Path $vs 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt')).Trim()
$redistVersion = (Get-Content (Join-Path $vs 'VC/Auxiliary/Build/Microsoft.VCRedistVersion.default.txt')).Trim()
$dumpbin = Join-Path $vs "VC/Tools/MSVC/$toolVersion/bin/Hostx64/x64/dumpbin.exe"
$crt = Get-ChildItem (Join-Path $vs "VC/Redist/MSVC/$redistVersion/x64") -Directory -Filter '*.CRT' | Select-Object -First 1
if (!$crt) { throw 'The x64 Visual C++ redistributable directory was not found.' }
$name = "MeshDevGUI-$Version-windows-x64"
$destination = Join-Path $root "output/packages/$name"
$zip = "$destination.zip"
if ((Test-Path -LiteralPath $destination) -or (Test-Path -LiteralPath $zip)) {
    throw "Package already exists; use a new version or move it aside: $destination"
}
New-Item -ItemType Directory -Path "$destination/licenses" -Force | Out-Null
foreach ($file in @('MeshDevGUI.exe', 'cuBQL_cuda_float3.dll')) {
    Copy-Item -LiteralPath (Join-Path $root "output/Release/$file") -Destination $destination
}
foreach ($file in @('msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')) {
    Copy-Item -LiteralPath (Join-Path $crt.FullName $file) -Destination $destination
}

# Inspect every packaged PE. Reject missing dependencies instead of depending on a developer PATH.
foreach ($binary in Get-ChildItem -LiteralPath $destination -File | Where-Object { $_.Extension -in '.exe', '.dll' }) {
    $imports = & $dumpbin /dependents $binary.FullName
    if ($LASTEXITCODE -ne 0) { throw "Cannot inspect $($binary.Name)" }
    foreach ($line in $imports) {
        if ($line -match '^\s+([A-Za-z0-9_.-]+\.dll)\s*$') {
            $dll = $matches[1]
            if ($dll -like 'api-ms-win-*' -or $dll -like 'ext-ms-win-*') { continue }
            if (!(Test-Path -LiteralPath (Join-Path $destination $dll)) -and
                !(Test-Path -LiteralPath (Join-Path $env:SystemRoot "System32/$dll"))) {
                throw "Unpackaged dependency: $($binary.Name) -> $dll"
            }
        }
    }
}

$dependencies = @{}
foreach ($dependency in @('cubql', 'owl', 'imguiax', 'imgui', 'glfw', 'glew', 'optix')) {
    $source = $cache['FETCHCONTENT_SOURCE_DIR_' + $dependency.ToUpperInvariant()]
    if (!$source) { $source = Join-Path $build "_deps/$dependency-src" }
    $license = @('LICENSE', 'LICENSE.txt', 'LICENSE.md', 'COPYING.txt') |
        ForEach-Object { Join-Path $source $_ } | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
    if (!$license) { throw "Missing license for $dependency" }
    Copy-Item -LiteralPath $license -Destination "$destination/licenses/$dependency.txt"
    if (Test-Path -LiteralPath (Join-Path $source '.git')) {
        $revision = & git -C $source rev-parse HEAD
        if ($LASTEXITCODE -ne 0) { throw "Cannot read revision for $dependency" }
        $dependencies[$dependency] = $revision
    } elseif ($dependency -eq 'glew') {
        # GLEW is downloaded as an archive; git -C would incorrectly find the parent repository.
        $versionParts = Get-Content (Join-Path $source 'config/version') | ForEach-Object {
            if ($_ -match '^GLEW_(MAJOR|MINOR|MICRO)\s*=\s*(\d+)') { $matches[2] }
        }
        $dependencies[$dependency] = @{
            version = ($versionParts -join '.')
            source_sha256 = (Get-FileHash (Join-Path $source 'src/glew.c') -Algorithm SHA256).Hash.ToLowerInvariant()
        }
    } else { throw "Cannot identify source revision for $dependency" }
}
Copy-Item -LiteralPath (Join-Path $root 'LICENSE') -Destination "$destination/LICENSE.txt"
Copy-Item -LiteralPath (Join-Path $CudaDirectory 'LICENSE') -Destination "$destination/licenses/CUDA.txt"
Copy-Item -LiteralPath (Join-Path $CudaDirectory 'EULA.txt') -Destination "$destination/licenses/CUDA-EULA.txt"
Copy-Item -LiteralPath (Join-Path $vs 'Licenses/1033/Redist.txt') -Destination "$destination/licenses/Microsoft-Redist.txt"
foreach ($header in @('tinyobj_loader.h', 'helper_math.h')) {
    $text = Get-Content -LiteralPath (Join-Path $root "libs/3rdParty/$header") -Raw
    $notice = [regex]::Match($text, '(?s)/\*.*?\*/').Value
    if (!$notice) { throw "Missing notice in $header" }
    [IO.File]::WriteAllText("$destination/licenses/$header.txt", $notice)
}
foreach ($header in @('stb_image.h', 'stb_image_write.h')) {
    $text = Get-Content -LiteralPath (Join-Path $root "libs/3rdParty/$header") -Raw
    $notice = [regex]::Match($text, '(?s)/\*\s*-+\s*This software is available under 2 licenses.*?\*/').Value
    if (!$notice) { throw "Missing notice in $header" }
    [IO.File]::WriteAllText("$destination/licenses/$header.txt", $notice)
}
Copy-Item -LiteralPath "$PSScriptRoot/README-Windows.txt" -Destination "$destination/README.txt"
Copy-Item -LiteralPath "$PSScriptRoot/THIRD-PARTY-NOTICES.txt" -Destination $destination
$commit = & git -C $root rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Cannot read source revision' }
$cuda = Get-Content -LiteralPath (Join-Path $CudaDirectory 'version.json') -Raw | ConvertFrom-Json
@{
    version = $Version; source_commit = $commit; configuration = 'Release'; platform = 'windows-x64'
    cuda = $cuda.cuda.version; cuda_runtime = $cuda.cuda_cudart.version
    msvc = $toolVersion; cuda_architectures = $cache['CMAKE_CUDA_ARCHITECTURES']
    dependencies = $dependencies
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath "$destination/BUILD-INFO.json" -Encoding UTF8
$hashes = Get-ChildItem -LiteralPath $destination -Recurse -File | Sort-Object FullName | ForEach-Object {
    $relative = $_.FullName.Substring($destination.Length + 1).Replace('\', '/')
    '{0}  {1}' -f (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant(), $relative
}
$hashes | Set-Content -LiteralPath "$destination/SHA256SUMS.txt" -Encoding ASCII
Compress-Archive -LiteralPath $destination -DestinationPath $zip -CompressionLevel Optimal
('{0}  {1}' -f (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant(), (Split-Path $zip -Leaf)) |
    Set-Content -LiteralPath "$zip.sha256" -Encoding ASCII
Write-Output $zip
