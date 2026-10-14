param(
 [Parameter(Mandatory=$true)][string]$PluginPath,
 [string]$ReportRoot = "build/full-qa-reports"
)
$ErrorActionPreference = "Stop"
$testerRun = 37432640438
$testerSha = "fd246146994384e301c735e7d860b53aa06b18c5"
$runText = gh api "repos/challanger2000/125A-Plugin-Tester-Final/actions/runs/$testerRun"
if ($LASTEXITCODE -ne 0) { throw "Cannot verify authoritative 125A Tester provenance" }
$run = ($runText -join "`n") | ConvertFrom-Json
if ($run.head_sha -ne $testerSha -or $run.conclusion -ne "success") {
 throw "125A Tester provenance or completed result mismatch"
}
$session = [Guid]::NewGuid().ToString("N")
$testerDirectory = Join-Path "build/qa-tester" $session
$reportDirectory = Join-Path $ReportRoot $session
New-Item -ItemType Directory -Path $reportDirectory -Force | Out-Null
gh run download $testerRun --repo challanger2000/125A-Plugin-Tester-Final `
 --name 125A-Plugin-Tester-Windows-x64 --dir $testerDirectory
if ($LASTEXITCODE -ne 0) { throw "125A Tester download failed" }
$tester = Get-ChildItem $testerDirectory -Recurse -File -Filter "125A_Plugin_Tester.exe" | Select-Object -First 1
if (-not $tester) { throw "Trusted tester executable missing" }
$plugin = (Resolve-Path -LiteralPath $PluginPath).Path
if ((Split-Path $plugin -Leaf) -ne "125A VoiceMorph.vst3") { throw "Wrong shipping package" }
$env:125A_PLUGIN_TESTER_REPORT_DIR = (Resolve-Path $reportDirectory).Path
$process = Start-Process -FilePath $tester.FullName `
 -ArgumentList @("--portable-test", ('"' + $plugin + '"')) -Wait -PassThru
$report = Get-ChildItem $reportDirectory -Recurse -File -Filter "*_125A_QA_Report.txt" | Select-Object -First 1
if (-not $report) { throw "125A Tester produced no report for this invocation" }
$text = Get-Content -LiteralPath $report.FullName -Raw
Write-Host $text
if ($process.ExitCode -ne 0 -or $text -notmatch '\b[1-9][0-9]* PASS\s*/\s*0 WARNING\s*/\s*0 FAIL\b') {
 throw "125A Tester failed or did not execute a positive clean test count"
}
$binary = Join-Path $plugin "Contents/x86_64-win/125A VoiceMorph.vst3"
$commit = (git rev-parse HEAD).Trim()
@{
 plugin_commit=$commit
 plugin_binary_sha256=(Get-FileHash -LiteralPath $binary -Algorithm SHA256).Hash
 tester_commit=$testerSha
 tester_run=$testerRun
 report=$report.Name
} | ConvertTo-Json | Set-Content (Join-Path $reportDirectory "provenance.json")
