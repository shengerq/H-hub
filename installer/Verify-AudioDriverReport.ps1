param([Parameter(Mandatory=$true)][string]$ReportPath)
$ErrorActionPreference='Stop'
try {
 $taskReport=Get-Content -LiteralPath $ReportPath -Raw -Encoding UTF8|ConvertFrom-Json
 foreach($taskField in @('ok','driverInstalled','nameUpdated','renameVerified')) {
  if($taskReport.$taskField -isnot [bool] -or -not $taskReport.$taskField){throw ('Driver report did not confirm '+$taskField)}
 }
 if($taskReport.driver -cne 'FXVAD' -or $taskReport.phase -cne 'complete' -or $taskReport.displayName -cne 'H hub Audio'){throw 'Driver identity, phase or display name did not match'}
 if($taskReport.renamedDevices -isnot [ValueType] -or $taskReport.renamedDevices -lt 1){throw 'No driver device name was verified'}
 if($taskReport.renamedEndpoints -isnot [ValueType] -or $taskReport.renamedEndpoints -lt 1){
  if($taskReport.endpointNamePending -isnot [bool] -or -not $taskReport.endpointNamePending -or $taskReport.rebootRequired -isnot [bool] -or -not $taskReport.rebootRequired){throw 'No audio endpoint name was verified and no reboot is pending'}
 }
 Write-Output 'EQ driver installation and H hub Audio naming report verified.'
 exit 0
} catch { Write-Error $_;exit 1 }
