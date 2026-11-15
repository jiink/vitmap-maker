# Serves the editor from localhost just long enough for the browser to load it, then exits.
# The page needs no server once loaded (all state lives in the tab), but ES modules
# refuse to load over file://, hence this tiny one-shot server.
#
# Usage: powershell -NoProfile -ExecutionPolicy Bypass -File tools\serve-once.ps1

param(
    [int]$Port = 0,
    [int]$IdleSeconds = 3,         # quit this long after the last request
    [int]$FirstRequestSeconds = 60, # give up if the browser never shows up
    [switch]$NoOpen               # do not launch a browser (for testing)
)

$ErrorActionPreference = 'Stop'
$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..')).TrimEnd('\') + '\'

$mime = @{
    '.html' = 'text/html; charset=utf-8'
    '.js'   = 'text/javascript; charset=utf-8'
    '.mjs'  = 'text/javascript; charset=utf-8'
    '.css'  = 'text/css; charset=utf-8'
    '.json' = 'application/json; charset=utf-8'
    '.md'   = 'text/markdown; charset=utf-8'
    '.svg'  = 'image/svg+xml'
    '.png'  = 'image/png'
    '.jpg'  = 'image/jpeg'
    '.ico'  = 'image/x-icon'
}

function Get-FreePort {
    $probe = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Loopback, 0)
    $probe.Start()
    $p = $probe.LocalEndpoint.Port
    $probe.Stop()
    return $p
}

if ($Port -eq 0) { $Port = Get-FreePort }
$prefix = "http://localhost:$Port/"
$listener = [System.Net.HttpListener]::new()
$listener.Prefixes.Add($prefix)
$listener.Start()
Write-Host "Serving $root at $prefix (closes automatically once the page has loaded)"
if (-not $NoOpen) { Start-Process $prefix }

function Send-Response($ctx) {
    $res = $ctx.Response
    try {
        $rel = [Uri]::UnescapeDataString($ctx.Request.Url.AbsolutePath).TrimStart('/')
        if ($rel -eq '') { $rel = 'index.html' }
        $path = [System.IO.Path]::GetFullPath((Join-Path $root $rel))
        if (-not $path.StartsWith($root, [StringComparison]::OrdinalIgnoreCase) -or -not (Test-Path $path -PathType Leaf)) {
            $res.StatusCode = 404
            return
        }
        $ext = [System.IO.Path]::GetExtension($path).ToLowerInvariant()
        $res.ContentType = if ($mime.ContainsKey($ext)) { $mime[$ext] } else { 'application/octet-stream' }
        $res.Headers['Cache-Control'] = 'no-store'
        $bytes = [System.IO.File]::ReadAllBytes($path)
        $res.ContentLength64 = $bytes.Length
        $res.OutputStream.Write($bytes, 0, $bytes.Length)
    } catch {
        $res.StatusCode = 500
    } finally {
        $res.Close()
    }
}

try {
    $timeout = $FirstRequestSeconds
    while ($true) {
        $task = $listener.GetContextAsync()
        if (-not $task.Wait([TimeSpan]::FromSeconds($timeout))) { break }
        Send-Response $task.Result
        $timeout = $IdleSeconds
    }
} finally {
    $listener.Stop()
    $listener.Close()
}
Write-Host "Page loaded; server closed."
