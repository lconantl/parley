$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new()
if (-not (Test-Path '.env') -and -not $env:BOT_TOKEN -and $args -notcontains '--check') {
    throw 'Скопируйте прежний .env в эту папку или заполните .env.mock и сохраните как .env.'
}
$parleyBinary = Join-Path $PSScriptRoot 'build-windows/Release/parley.exe'
if (-not (Test-Path $parleyBinary)) {
    if (-not $env:VCPKG_ROOT) {
        throw 'Задайте VCPKG_ROOT — путь к установленному vcpkg. Инструкция есть в README.md.'
    }
    & cmake -S . -B build-windows "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" -DBUILD_TESTING=OFF
    if ($LASTEXITCODE -ne 0) { throw 'Не удалось настроить сборку.' }
    & cmake --build build-windows --config Release --parallel 2
    if ($LASTEXITCODE -ne 0) { throw 'Не удалось собрать бота.' }
}
& $parleyBinary @args
exit $LASTEXITCODE
