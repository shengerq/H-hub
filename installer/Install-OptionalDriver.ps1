param([ValidateSet('audio','pawnio')][string]$Driver,[switch]$VerifyOnly)
$ErrorActionPreference='Stop'
[Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12
$driverWork=Join-Path ([IO.Path]::GetTempPath()) ('HhubDriver-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $driverWork | Out-Null
try {
 if($Driver -eq 'pawnio') {
  if(-not $VerifyOnly -and (Get-Service PawnIO -ErrorAction SilentlyContinue)){exit 0}
  $driverExe=Join-Path $driverWork 'PawnIO_setup.exe'
  Invoke-WebRequest 'https://github.com/namazso/PawnIO.Setup/releases/download/2.2.0/PawnIO_setup.exe' -UseBasicParsing -OutFile $driverExe
  if((Get-FileHash $driverExe -Algorithm SHA256).Hash -ne '1f519a22e47187f70a1379a48ca604981c4fcf694f4e65b734aaa74a9fba3032'){throw 'PawnIO download hash mismatch'}
  $publisherPattern='namazso'
 } else {
  if(-not $VerifyOnly -and (Get-CimInstance Win32_PnPEntity | Where-Object {$_.HardwareID -contains 'VBAudioVACWDM'})){exit 0}
  $driverZip=Join-Path $driverWork 'VBCABLE.zip'
  Invoke-WebRequest 'https://download.vb-audio.com/Download_CABLE/VBCABLE_Driver_Pack45.zip' -UseBasicParsing -OutFile $driverZip
  Expand-Archive -LiteralPath $driverZip -DestinationPath (Join-Path $driverWork 'cable')
  $driverExe=(Get-ChildItem (Join-Path $driverWork 'cable') -Filter 'VBCABLE_Setup_x64.exe' -Recurse | Select-Object -First 1).FullName
  if(-not $driverExe){throw 'Official x64 audio installer missing'}
  $publisherPattern='VB.Audio|Vincent Burel|BUREL VINCENT'
 }
 $driverSignature=Get-AuthenticodeSignature -LiteralPath $driverExe
 if($driverSignature.Status -ne 'Valid' -or $driverSignature.SignerCertificate.Subject -notmatch $publisherPattern){throw 'Driver publisher signature validation failed'}
 if($VerifyOnly){Write-Output "$Driver verified: $($driverSignature.SignerCertificate.Subject)";exit 0}
 $driverProcess=Start-Process -FilePath $driverExe -Verb RunAs -Wait -PassThru
 if($driverProcess.ExitCode -notin @(0,3010)){throw "Driver installer returned $($driverProcess.ExitCode)"}
 exit $driverProcess.ExitCode
} catch {
 if($VerifyOnly){Write-Error $_;exit 1}
 Add-Type -AssemblyName System.Windows.Forms
 [Windows.Forms.MessageBox]::Show("驱动安装未完成：$($_.Exception.Message)`n主程序仍可使用，可稍后从 H hub 驱动检查入口重试。",'H hub') | Out-Null
 exit 1
}
