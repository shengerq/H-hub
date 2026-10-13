param([string]$JsonJar="$PSScriptRoot\..\..\..\work\json-20250517.jar",[string]$ReportPath,[string]$ZxingJar="$PSScriptRoot\..\..\..\work\zxing-core-3.5.4.jar")
$ErrorActionPreference='Stop'
$taskNativeReport=$ReportPath
$JsonJar=[IO.Path]::GetFullPath($JsonJar)
$ZxingJar=[IO.Path]::GetFullPath($ZxingJar)
$taskClasspath=$JsonJar+';'+$ZxingJar
if(-not (Test-Path -LiteralPath $JsonJar)){throw 'Pass -JsonJar with an org.json JVM implementation for this test'}
$taskTests=Join-Path $PSScriptRoot 'build\tests'
New-Item -ItemType Directory -Force $taskTests | Out-Null
& javac -encoding UTF-8 --release 8 -cp $taskClasspath -d $taskTests (Join-Path $PSScriptRoot 'src\cn\hhub\mobile\HubClient.java') (Join-Path $PSScriptRoot 'src\cn\hhub\mobile\PairingQr.java') (Join-Path $PSScriptRoot 'tests\NativeClientTest.java')
if($LASTEXITCODE -ne 0){throw 'Native network test compilation failed'}
. (Join-Path $PSScriptRoot '..\..\source\test-ipc.ps1')
Add-Type -TypeDefinition @'
using System;using System.Net.Http;using System.Security.Cryptography;
public class AndroidPairClient:IDisposable {
 readonly HttpClient client;
 public AndroidPairClient(string fingerprint){var handler=new HttpClientHandler{UseProxy=false};handler.ServerCertificateCustomValidationCallback=(r,c,ch,e)=>{using(var s=SHA256.Create())return BitConverter.ToString(s.ComputeHash(c.RawData)).Replace("-","")==fingerprint;};client=new HttpClient(handler);client.Timeout=TimeSpan.FromSeconds(8);}
 public string Post(string address,string data){using(var body=new StringContent(data,System.Text.Encoding.UTF8,"application/json"))using(var reply=client.PostAsync(address,body).GetAwaiter().GetResult()){reply.EnsureSuccessStatusCode();return reply.Content.ReadAsStringAsync().GetAwaiter().GetResult();}}
 public void Dispose(){client.Dispose();}
}
'@
$taskId=''
$taskClient=$null
try {
 $taskService=Invoke-HhubRequest 'phoneStart' @{lan=$false;port=43829}
 if(-not $taskService.ok){throw $taskService.error}
 $taskRuntime=$taskService.state.phoneRuntime
 $taskAddress='https://127.0.0.1:43829'
 $taskClient=[AndroidPairClient]::new($taskRuntime.fingerprint)
 $taskPending=$taskClient.Post($taskAddress+'/api/pair',(@{name='Android native transport test';code=$taskRuntime.code}|ConvertTo-Json -Compress))|ConvertFrom-Json
 $taskId=$taskPending.id
 $taskApproved=Invoke-HhubRequest 'phoneApprove' @{id=$taskId;control=$false}
 if(-not $taskApproved.ok){throw $taskApproved.error}
 $taskApproval=$taskClient.Post($taskAddress+'/api/pair-status',(@{id=$taskId;pollSecret=$taskPending.pollSecret;native=$true}|ConvertTo-Json -Compress))|ConvertFrom-Json
 if(-not $taskApproval.token){throw 'No native token in approved pairing'}
 $taskStart=[Diagnostics.ProcessStartInfo]::new('java')
 $taskStart.ArgumentList.Add('-cp');$taskStart.ArgumentList.Add($taskTests+';'+$taskClasspath);$taskStart.ArgumentList.Add('cn.hhub.mobile.NativeClientTest')
 $taskStart.UseShellExecute=$false;$taskStart.CreateNoWindow=$true;$taskStart.RedirectStandardInput=$true;$taskStart.RedirectStandardOutput=$true;$taskStart.RedirectStandardError=$true
 $taskJava=[Diagnostics.Process]::Start($taskStart)
 $taskJava.StandardInput.WriteLine((@{address=$taskAddress;fingerprint=$taskRuntime.fingerprint;token=$taskApproval.token;pairingUri=$taskRuntime.pairingUri}|ConvertTo-Json -Compress));$taskJava.StandardInput.Close()
 $taskOutput=$taskJava.StandardOutput.ReadToEndAsync();$taskError=$taskJava.StandardError.ReadToEndAsync()
 if(-not $taskJava.WaitForExit(30000)){$taskJava.Kill();throw 'Native transport test timed out'}
 if($taskJava.ExitCode -ne 0){throw $taskError.GetAwaiter().GetResult()}
 $taskText=$taskOutput.GetAwaiter().GetResult()
 if($taskNativeReport){[IO.File]::WriteAllText($taskNativeReport,$taskText)}
 $taskText
} finally {
 if($taskId){Invoke-HhubRequest 'phoneRevoke' @{id=$taskId}|Out-Null}
 Invoke-HhubRequest 'phoneStop'|Out-Null
 if($taskClient){$taskClient.Dispose()}
}
