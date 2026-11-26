$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Push-Location $projectRoot
try {
    $zigPath = Join-Path $projectRoot '.venv\Lib\site-packages\ziglang\zig.exe'
    if (-not (Test-Path -LiteralPath $zigPath)) {
        throw 'Install test tools first: .\.venv\Scripts\python.exe -m pip install -r tools\requirements.txt'
    }
    New-Item -ItemType Directory -Force -Path build | Out-Null
    & $zigPath c++ -std=c++11 -Wall -Wextra -Werror -I tests/stubs -I include -I lib/NpkProtocol tests/test_npk.cpp lib/NpkProtocol/NpkProtocol.cpp src/Sensor.cpp -o build/test_npk.exe
    if ($LASTEXITCODE -ne 0) { throw 'Test compilation failed' }
    & .\build\test_npk.exe
    if ($LASTEXITCODE -ne 0) { throw 'Protocol/state-machine tests failed' }
} finally {
    Pop-Location
}
