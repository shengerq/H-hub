param([string]$ReportPath)
$ErrorActionPreference='Stop'
$taskPhoneReport=$ReportPath
$taskTestPort=if($env:HHUB_TEST_PROFILE -eq 'phone-tests'){43828}else{43827}
. (Join-Path $PSScriptRoot 'test-ipc.ps1')
$taskNativeSnapshot=Invoke-HhubRequest 'snapshot'
if(-not @($taskNativeSnapshot.state.plugins | Where-Object { $_.id -eq 'phone' -and $_.installed }).Count){
 $taskNativeApp=Split-Path (Get-Process -Id ([int]$taskNativeSnapshot.state.corePid)).Path
 $taskNativeManifest=Get-Content -LiteralPath (Join-Path $env:HHUB_NATIVE_TEST_PACKAGES 'Hhub-Phone-Remote-1.0.9.hhub') -Raw -Encoding UTF8 | ConvertFrom-Json
 $taskNativeInstall=Invoke-HhubRequest 'importPlugin' @{manifest=$taskNativeManifest}
 if(-not $taskNativeInstall.ok){throw $taskNativeInstall.error}
}

Add-Type -TypeDefinition @'
using System;using System.Net;using System.Net.Http;using System.Security.Cryptography;using System.Security.Cryptography.X509Certificates;using System.Text;
public sealed class HhubPinnedClient:IDisposable {
 readonly HttpClient client;public HhubPinnedClient(string fingerprint){var handler=new HttpClientHandler {UseProxy=false,CookieContainer=new CookieContainer(),UseCookies=true};handler.ServerCertificateCustomValidationCallback=(request,cert,chain,errors)=>{using(var sha=SHA256.Create()){return BitConverter.ToString(sha.ComputeHash(cert.RawData)).Replace("-","")==fingerprint;}};client=new HttpClient(handler);client.Timeout=TimeSpan.FromSeconds(8);}
 public string[] Send(string url,string body=null){using(var request=new HttpRequestMessage(String.IsNullOrEmpty(body)?HttpMethod.Get:HttpMethod.Post,url)){if(!String.IsNullOrEmpty(body))request.Content=new StringContent(body,Encoding.UTF8,"application/json");using(var response=client.SendAsync(request).GetAwaiter().GetResult())return new[]{((int)response.StatusCode).ToString(),response.Content.ReadAsStringAsync().GetAwaiter().GetResult()};}}
 public void Token(string token){client.DefaultRequestHeaders.Authorization=new System.Net.Http.Headers.AuthenticationHeaderValue("Bearer",token);} public void Dispose(){client.Dispose();}
}
'@
$taskResults=[Collections.Generic.List[string]]::new()
function Assert-Phone($Condition,[string]$Message){if(-not $Condition){throw $Message};$taskResults.Add('PASS '+$Message)}
function Phone-Http([string]$Path,$Body=$null){$payload=if($null -ne $Body){$Body|ConvertTo-Json -Compress -Depth 10}else{$null};$raw=$taskClient.Send($taskBase+$Path,$payload);[pscustomobject]@{status=[int]$raw[0];text=$raw[1];json=$(if($Path -like '/api/*'){$raw[1]|ConvertFrom-Json}else{$null})}}
$taskStart=Invoke-HhubRequest 'phoneStart' @{lan=$false;port=$taskTestPort}
if(-not $taskStart.ok){throw $taskStart.error}
$taskService=$taskStart.state.phoneRuntime
$taskTestPhoneBinary=Join-Path (Split-Path (Get-Process -Id ([int]$taskStart.state.corePid)).Path) 'Hhub.Phone.exe'
Assert-Phone ($taskService.pairingUri -like 'hhub://pair?v=2&address=*' -and $taskService.pairingUri -like '*&expires=*') 'Desktop generates a versioned expiring Android pairing URI'
$taskBase='https://127.0.0.1:'+$taskTestPort
$taskClient=[HhubPinnedClient]::new($taskService.fingerprint)
$taskDeviceId=''
try {
 $page=Phone-Http '/'
 Assert-Phone ($page.status -eq 200 -and $page.text -like '*H hub*') 'Pinned HTTPS serves the mobile companion page'
 $denied=Phone-Http '/api/status'
 Assert-Phone ($denied.status -eq 401) 'Unauthenticated clients cannot read computer status'
 $wrong=Phone-Http '/api/pair' @{name='Phone integration test';code='invalid'}
 Assert-Phone ($wrong.status -eq 403) 'Incorrect pairing code is rejected'
 $pending=Phone-Http '/api/pair' @{name='Phone integration test';code=$taskService.code}
 Assert-Phone ($pending.status -eq 202) 'Correct code creates a pending request, without granting access'
 $taskDeviceId=$pending.json.id
 $state=Invoke-HhubRequest 'phoneState'
 Assert-Phone ($taskDeviceId -in $state.state.phoneRuntime.pending.id) 'Desktop sees the actual client pairing request'
 $poll=Phone-Http '/api/pair-status' @{id=$taskDeviceId;pollSecret=$pending.json.pollSecret;native=$true}
 Assert-Phone ($poll.json.status -eq 'pending') 'Client remains pending until desktop approval'
 $approved=Invoke-HhubRequest 'phoneApprove' @{id=$taskDeviceId;control=$false}
 Assert-Phone $approved.ok 'Desktop approves a read-only device'
 $poll=Phone-Http '/api/pair-status' @{id=$taskDeviceId;pollSecret=$pending.json.pollSecret;native=$true}
 Assert-Phone ($poll.json.status -eq 'approved') 'Approved client receives a secure authentication cookie'
 $view=Phone-Http '/api/status'
 Assert-Phone ($poll.json.token.Length -ge 32) 'Approved native pairing returns a one-time Bearer token'
 $taskNative=[HhubPinnedClient]::new($taskService.fingerprint)
 try {
  $taskNative.Token($poll.json.token)
  $taskNativeReply=$taskNative.Send($taskBase+'/api/status',$null)
  Assert-Phone ([int]$taskNativeReply[0] -eq 200) 'Native Bearer client authenticates without browser cookies'
  $taskNativeAction=$taskNative.Send($taskBase+'/api/action','{"kind":"deviceVibrate","id":"missing"}')
  Assert-Phone ([int]$taskNativeAction[0] -eq 403) 'Native read-only Bearer client cannot control hardware'
 } finally {$taskNative.Dispose()}
 $claimed=Phone-Http '/api/pair-status' @{id=$taskDeviceId;pollSecret=$pending.json.pollSecret;native=$true}
 Assert-Phone ($claimed.status -eq 403) 'Native token cannot be claimed twice'
 Assert-Phone ($view.status -eq 200 -and $view.json.ok -and $view.json.data.monitor) 'Authenticated client receives actual sanitized computer telemetry'
 Assert-Phone (-not ($view.json.PSObject.Properties.Name -contains 'state')) 'Remote response does not expose the full local configuration'
 $blocked=Phone-Http '/api/action' @{kind='scene';id='desktop'}
 Assert-Phone ($blocked.status -eq 403) 'Read-only pairing cannot invoke remote actions'
 Invoke-HhubRequest 'phonePermission' @{id=$taskDeviceId;control=$true} | Out-Null
 $view=Phone-Http '/api/status'
 Assert-Phone $view.json.control 'Desktop permission changes apply to the existing session'
 Assert-Phone ($view.json.data.sceneDemo -eq $false -and $null -ne $view.json.data.profiles -and $view.json.data.presets.Count -ge 13) 'Mobile receives upgraded scene profiles and EQ presets'
 $scene=Phone-Http '/api/action' @{kind='scene';id='desktop'}
 Assert-Phone ($scene.json.ok -and $scene.json.data.sceneLocked) 'Native scene selection uses the upgraded model and manual lock'
 $auto=Phone-Http '/api/action' @{kind='sceneAuto'}
 Assert-Phone ($auto.json.ok -and -not $auto.json.data.sceneLocked) 'Native client can restore automatic foreground scenes'
 $profile=Phone-Http '/api/action' @{kind='profileSelect';id='missing'}
 Assert-Phone (-not $profile.json.ok) 'Unknown saved device profile cannot be selected'
 Assert-Phone ($null -ne $view.json.data.devices) 'Mobile status includes real peripheral collection'
 $invalidEq=Phone-Http '/api/action' @{kind='eqSave';id='missing';enabled=$true;bands=@(0)*12;preamp=0;autoPreamp=$true}
 Assert-Phone ($invalidEq.json.ok -eq $false) 'Unknown EQ configuration is rejected'
 $invalidDevice=Phone-Http '/api/action' @{kind='deviceVibrate';id='missing'}
 Assert-Phone ($invalidDevice.json.ok -eq $false) 'Unsupported peripheral cannot receive vibration'
 $stillAlive=Phone-Http '/api/status'
 Assert-Phone ($stillAlive.status -eq 200) 'Failed remote controls preserve connection'
 $arbitrary=Phone-Http '/api/action' @{kind='windowsSet';id='gameMode';enabled=$true}
 Assert-Phone ($arbitrary.status -eq 400) 'Unlisted system operations cannot be forwarded'
 $state=Invoke-HhubRequest 'phoneState'
 $device=$state.state.phoneRuntime.phones | Where-Object id -eq $taskDeviceId
 Assert-Phone ($device.status -eq '在线') 'Online state is based on authenticated traffic'
 Invoke-HhubRequest 'phoneStop' | Out-Null
 $taskProcess=Get-Process Hhub.Phone | Where-Object {$_.Path -eq $taskTestPhoneBinary};$taskProcess | Stop-Process;foreach($taskExited in $taskProcess){$taskExited.WaitForExit(2000) | Out-Null}
 $restored=Invoke-HhubRequest 'phoneStart' @{lan=$false;port=$taskTestPort}
 $null=Assert-Phone $restored.ok 'Phone service restarts under the same user'
 $view=Phone-Http '/api/status'
 Assert-Phone ($view.status -eq 200 -and $view.json.control) 'Encrypted authorization and certificate survive a service restart'
 Invoke-HhubRequest 'phoneRevoke' @{id=$taskDeviceId} | Out-Null
 $revoked=Phone-Http '/api/status'
 Assert-Phone ($revoked.status -eq 401) 'Revocation invalidates the previously issued cookie immediately'
 $state=Invoke-HhubRequest 'phoneState'
 Assert-Phone ($taskDeviceId -notin $state.state.phoneRuntime.phones.id) 'Revoked device disappears from the desktop authorization list'
} finally {
 if($taskDeviceId){Invoke-HhubRequest 'phoneRevoke' @{id=$taskDeviceId} | Out-Null}
 Invoke-HhubRequest 'phoneStop' | Out-Null
 $taskClient.Dispose()
}
$taskResults.Add('Protocol tested with a pinned HTTPS client on loopback. Physical Android/iPhone access through the LAN and private-network firewall rule still requires handset/network verification.')
if($taskPhoneReport){[IO.File]::WriteAllLines($taskPhoneReport,$taskResults,[Text.UTF8Encoding]::new($true))}
$taskResults
