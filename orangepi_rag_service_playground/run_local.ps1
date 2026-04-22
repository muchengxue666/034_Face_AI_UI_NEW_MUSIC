$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Path $PSScriptRoot -Parent
Set-Location $repoRoot

if (-not $env:ORANGEPI_MEMORY_EMBED_BACKEND) {
    $env:ORANGEPI_MEMORY_EMBED_BACKEND = "hashing"
}

if (-not $env:ORANGEPI_MEMORY_HOST) {
    $env:ORANGEPI_MEMORY_HOST = "127.0.0.1"
}

if (-not $env:ORANGEPI_MEMORY_PORT) {
    $env:ORANGEPI_MEMORY_PORT = "8765"
}

Write-Host "启动香橙派记忆中枢本地服务..."
Write-Host "地址: http://$($env:ORANGEPI_MEMORY_HOST):$($env:ORANGEPI_MEMORY_PORT)/playground"
Write-Host "向量后端: $($env:ORANGEPI_MEMORY_EMBED_BACKEND)"

python -m uvicorn orangepi_rag_service.app:app --host $env:ORANGEPI_MEMORY_HOST --port $env:ORANGEPI_MEMORY_PORT --reload
