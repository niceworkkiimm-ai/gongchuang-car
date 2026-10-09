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
    $branch = & git branch --show-current
    if ($LASTEXITCODE -ne 0 -or $branch -ne 'main') { throw 'Run this backup from the main branch.' }
    $remotes = @('origin', 'team')
    # Stop before copying if a collaborator has uploaded changes we do not have.
    foreach ($remote in $remotes) {
        & git remote get-url $remote
        if ($LASTEXITCODE -ne 0) { throw "Missing remote: $remote. Configure both repositories before backup." }
        & git fetch $remote main
        if ($LASTEXITCODE -ne 0) { throw "Cannot fetch $remote/main; nothing synced or committed." }
        & git merge-base --is-ancestor "refs/remotes/$remote/main" HEAD
        if ($LASTEXITCODE -ne 0) { throw "$remote/main contains changes not in local main. Review and integrate them before backup; do not force push." }
    }
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
    $uploaded = @()
    $failed = @()
    foreach ($remote in $remotes) {
        & git push $remote main
        if ($LASTEXITCODE -eq 0) { $uploaded += $remote }
        else { $failed += $remote }
    }
    if ($failed.Count) {
        throw "Upload incomplete. Succeeded: $($uploaded -join ', '); failed: $($failed -join ', '). Local commit is retained; rerun after resolving the failure."
    }
    Write-Host 'Backup uploaded successfully to origin and team.'
} finally {
    Pop-Location
}
