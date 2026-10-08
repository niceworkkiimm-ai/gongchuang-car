param([string]$Message = '')
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
# Reuse Windows' configured proxy for Git; no system settings are changed.
$systemProxy = [System.Net.WebRequest]::GetSystemWebProxy()
$githubUri = [Uri]'https://github.com/'
if (-not $systemProxy.IsBypassed($githubUri)) {
    $env:HTTPS_PROXY = $systemProxy.GetProxy($githubUri).AbsoluteUri
    $env:HTTP_PROXY = $env:HTTPS_PROXY
}
Push-Location $repo
try {
    & python (Join-Path $PSScriptRoot 'sync_from_local.py')
    if ($LASTEXITCODE -ne 0) { throw 'Sync failed; nothing committed.' }
    & git add --all
    if ($LASTEXITCODE -ne 0) { throw 'git add failed.' }
    & git diff --cached --quiet
    $diffStatus = $LASTEXITCODE
    if ($diffStatus -eq 1) {
        if (-not $Message) { $Message = 'Save project and conversation ' + (Get-Date -Format 'yyyy-MM-dd HH:mm:ss') }
        & git commit -m $Message
        if ($LASTEXITCODE -ne 0) { throw 'Commit failed.' }
    } elseif ($diffStatus -ne 0) { throw 'Cannot inspect staged changes.' }
    & git push origin main
    if ($LASTEXITCODE -ne 0) { throw 'Upload failed. Local commit is retained; rerun this script later.' }
    Write-Host 'Backup uploaded successfully.'
} finally {
    Pop-Location
}
