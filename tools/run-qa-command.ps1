param(
 [Parameter(Mandatory=$true)][string]$Executable,
 [Parameter(Mandatory=$true)][string[]]$CommandArgs
)
# Preserve every original test gate and exit status while making diagnostics
# available in Actions annotations when API/log access is unavailable.
$qaLogRoot = "build/qa-command-logs"
New-Item -ItemType Directory -Path $qaLogRoot -Force | Out-Null
$qaLog = Join-Path $qaLogRoot ([Guid]::NewGuid().ToString("N") + ".log")
$qaExit = 0
try {
 $ErrorActionPreference = "Continue"
 & $Executable @CommandArgs 2>&1 | Tee-Object -FilePath $qaLog
 $qaExit = $LASTEXITCODE
} catch {
 $_ | Out-String | Tee-Object -FilePath $qaLog -Append | Write-Host
 $qaExit = 1
}
$ErrorActionPreference = "Stop"
function Escape-Annotation([string]$Text) {
 return ($Text -replace "%", "%25" -replace "`r", "%0D" -replace "`n", "%0A")
}
if ($qaExit -ne 0) {
 $junit = "build/windows-tests.xml"
 if ($Executable -eq "ctest" -and (Test-Path -LiteralPath $junit)) {
  [xml]$results = Get-Content -LiteralPath $junit -Raw
  foreach ($test in $results.SelectNodes("//testcase[failure]")) {
   $name = $test.GetAttribute("name")
   $output = $test.SelectSingleNode("system-out")
   $detail = if ($output) { $output.InnerText } else { $test.InnerText }
   Write-Host ("::error::" + (Escape-Annotation ($name + "`n" + $detail)))
  }
 } else {
  $detail = (Get-Content -LiteralPath $qaLog | Select-Object -Last 100) -join "`n"
  Write-Host ("::error::" + (Escape-Annotation ($Executable + " exit=" + $qaExit + "`n" + $detail)))
 }
 exit $qaExit
}
