param(
    [string]$BindHost = "",
    [int]$Port = 0
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Path $PSScriptRoot -Parent
Set-Location $repoRoot

function Test-TcpPortAvailable {
    param(
        [string]$Address,
        [int]$TcpPort
    )

    $listener = $null
    try {
        $ipAddress = [System.Net.IPAddress]::Parse($Address)
        $listener = [System.Net.Sockets.TcpListener]::new($ipAddress, $TcpPort)
        $listener.Start()
        return $true
    } catch {
        return $false
    } finally {
        if ($null -ne $listener) {
            $listener.Stop()
        }
    }
}

function Get-AvailablePort {
    param(
        [string]$Address,
        [int]$PreferredPort
    )

    if (Test-TcpPortAvailable -Address $Address -TcpPort $PreferredPort) {
        return $PreferredPort
    }

    for ($candidate = $PreferredPort + 1; $candidate -le ($PreferredPort + 100); $candidate++) {
        if (Test-TcpPortAvailable -Address $Address -TcpPort $candidate) {
            Write-Warning "端口 $PreferredPort 当前不可用，已自动改用 $candidate。"
            return $candidate
        }
    }

    throw "没有找到可用端口，请手动指定：.\orangepi_rag_service\run_local.ps1 -Port 8766"
}

function Get-LanUrls {
    param(
        [int]$TcpPort
    )

    $urls = @()
    $addresses = Get-NetIPAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue |
        Where-Object {
            $_.IPAddress -notlike "127.*" -and
            $_.IPAddress -notlike "169.254.*" -and
            $_.PrefixOrigin -ne "WellKnown"
        } |
        Select-Object -ExpandProperty IPAddress -Unique

    foreach ($address in $addresses) {
        $urls += "http://$address`:$TcpPort/admin"
    }
    return $urls
}

if (-not $env:ORANGEPI_MEMORY_EMBED_BACKEND) {
    $env:ORANGEPI_MEMORY_EMBED_BACKEND = "hashing"
}

if ($BindHost) {
    $env:ORANGEPI_MEMORY_HOST = $BindHost
} elseif (-not $env:ORANGEPI_MEMORY_HOST) {
    $env:ORANGEPI_MEMORY_HOST = "127.0.0.1"
}

if ($Port -gt 0) {
    $env:ORANGEPI_MEMORY_PORT = [string]$Port
} elseif (-not $env:ORANGEPI_MEMORY_PORT) {
    $env:ORANGEPI_MEMORY_PORT = "8765"
}

$selectedPort = Get-AvailablePort -Address $env:ORANGEPI_MEMORY_HOST -PreferredPort ([int]$env:ORANGEPI_MEMORY_PORT)
$env:ORANGEPI_MEMORY_PORT = [string]$selectedPort

Write-Host "启动香橙派记忆中枢本地服务..."
Write-Host "地址: http://$($env:ORANGEPI_MEMORY_HOST):$($env:ORANGEPI_MEMORY_PORT)/admin"
if ($env:ORANGEPI_MEMORY_HOST -eq "0.0.0.0") {
    $lanUrls = Get-LanUrls -TcpPort ([int]$env:ORANGEPI_MEMORY_PORT)
    if ($lanUrls.Count -gt 0) {
        Write-Host "手机访问地址:"
        foreach ($url in $lanUrls) {
            Write-Host "  $url"
        }
    }
}
Write-Host "向量后端: $($env:ORANGEPI_MEMORY_EMBED_BACKEND)"

python -m uvicorn orangepi_rag_service.app:app --host $env:ORANGEPI_MEMORY_HOST --port $env:ORANGEPI_MEMORY_PORT --reload
