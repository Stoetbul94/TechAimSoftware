<#
  Phase 3 WP0 - repeatable performance measurement for the lane application.

  Measures, for ONE launch of the given executable:
    - time from process start to the main window appearing
    - working set / private bytes after startup and after an idle period
    - CPU used during the idle period: whole process, and per thread
      (the busiest threads, so GUI vs acquisition can be told apart)
    - optional: memory after a manual or scripted screen-switching phase

  It only READS process counters. It changes no configuration and writes only
  its own result file. Run it on the SETA hardware exactly as on a laptop:

    powershell -ExecutionPolicy Bypass -File tools\perf\measure.ps1 `
        -Exe "C:\path\SETA.exe" -Label "laptop-demo" -IdleSeconds 30

  -SwitchSeconds N  keeps sampling for N more seconds while someone (or a test
                    driver) switches screens, then reports memory growth.
  -Runs N           repeats the whole measurement N times (default 3) and
                    prints min / median / max, because single runs are noisy.
#>
param(
    [Parameter(Mandatory = $true)] [string] $Exe,
    [string] $Label = "run",
    [int] $IdleSeconds = 30,
    [int] $SwitchSeconds = 0,
    [int] $Runs = 3,
    [string[]] $AppArgs = @(),
    [string] $OutFile = ""
)
$ErrorActionPreference = 'Stop'
$exeDir  = Split-Path -Parent $Exe
$name    = [IO.Path]::GetFileNameWithoutExtension($Exe)
$cores   = [Environment]::ProcessorCount

function Measure-Once {
    Get-Process $name -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 2
    $sw = [Diagnostics.Stopwatch]::StartNew()
    $p = if ($AppArgs.Count) { Start-Process $Exe -ArgumentList $AppArgs -WorkingDirectory $exeDir -PassThru }
         else { Start-Process $Exe -WorkingDirectory $exeDir -PassThru }
    while (-not $p.HasExited -and $p.MainWindowHandle -eq 0 -and $sw.Elapsed.TotalSeconds -lt 60) {
        Start-Sleep -Milliseconds 50; $p.Refresh()
    }
    $windowMs = $sw.ElapsedMilliseconds
    Start-Sleep -Seconds 5                       # let startup settle
    $p.Refresh()
    $ws0 = $p.WorkingSet64; $pr0 = $p.PrivateMemorySize64
    $cpu0 = $p.TotalProcessorTime.TotalMilliseconds
    $thr0 = @{}; foreach ($t in $p.Threads) { try { $thr0[$t.Id] = $t.TotalProcessorTime.TotalMilliseconds } catch {} }
    Start-Sleep -Seconds $IdleSeconds
    $p.Refresh()
    $cpuIdle = $p.TotalProcessorTime.TotalMilliseconds - $cpu0
    $threads = foreach ($t in $p.Threads) {
        try {
            $d = $t.TotalProcessorTime.TotalMilliseconds - ($(if ($thr0.ContainsKey($t.Id)) { $thr0[$t.Id] } else { 0 }))
            [pscustomobject]@{ Id = $t.Id; Ms = [math]::Round($d) }
        } catch {}
    }
    $top = $threads | Sort-Object Ms -Descending | Select-Object -First 4
    $r = [ordered]@{
        windowMs        = $windowMs
        workingSetMB    = [math]::Round($ws0 / 1MB, 1)
        privateMB       = [math]::Round($pr0 / 1MB, 1)
        idleCpuPctCore  = [math]::Round(100 * $cpuIdle / ($IdleSeconds * 1000), 1)
        idleCpuPctTotal = [math]::Round(100 * $cpuIdle / ($IdleSeconds * 1000 * $cores), 2)
        workingSetAfterIdleMB = [math]::Round($p.WorkingSet64 / 1MB, 1)
        topThreadsMs    = ($top | ForEach-Object { "$($_.Id):$($_.Ms)" }) -join ' '
        threadCount     = $p.Threads.Count
    }
    if ($SwitchSeconds -gt 0) {
        Write-Host "  switching phase: $SwitchSeconds s - switch screens now"
        $samples = @()
        for ($i = 0; $i -lt $SwitchSeconds; $i++) { Start-Sleep 1; $p.Refresh(); $samples += $p.PrivateMemorySize64 }
        $r.privateAfterSwitchMB = [math]::Round($samples[-1] / 1MB, 1)
        $r.privatePeakSwitchMB  = [math]::Round(($samples | Measure-Object -Maximum).Maximum / 1MB, 1)
        $r.privateGrowthPct     = [math]::Round(100 * ($samples[-1] - $pr0) / $pr0, 1)
    }
    Stop-Process -Id $p.Id -Force
    return [pscustomobject]$r
}

$all = @()
for ($i = 1; $i -le $Runs; $i++) {
    Write-Host "run $i / $Runs"
    $all += Measure-Once
}
function Stat($field) {
    $v = $all | ForEach-Object { $_.$field } | Where-Object { $_ -ne $null } | Sort-Object
    if (-not $v) { return "" }
    "{0} / {1} / {2}" -f $v[0], $v[[int][math]::Floor($v.Count / 2)], $v[-1]
}
$summary = [ordered]@{
    label = $Label; exe = $Exe; date = (Get-Date -Format s); cores = $cores
    machine = $env:COMPUTERNAME; runs = $Runs; idleSeconds = $IdleSeconds
    'windowMs (min/med/max)'          = Stat 'windowMs'
    'workingSetMB (min/med/max)'      = Stat 'workingSetMB'
    'privateMB (min/med/max)'         = Stat 'privateMB'
    'idleCpuPctCore (min/med/max)'    = Stat 'idleCpuPctCore'
    'idleCpuPctTotal (min/med/max)'   = Stat 'idleCpuPctTotal'
    'privateGrowthPct (min/med/max)'  = Stat 'privateGrowthPct'
}
$all | Format-Table -AutoSize | Out-String -Width 220 | Write-Host
$summary.GetEnumerator() | ForEach-Object { "{0,-34} {1}" -f $_.Key, $_.Value } | Write-Host
if ($OutFile) {
    @{ summary = $summary; runs = $all } | ConvertTo-Json -Depth 4 | Set-Content $OutFile -Encoding utf8
    Write-Host "written: $OutFile"
}
