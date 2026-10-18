param([string]$ReportPath)
$ErrorActionPreference='Stop'
$taskPipe='HhubPrototype.v1.'+[Security.Principal.WindowsIdentity]::GetCurrent().User.Value
if($env:HHUB_TEST_PROFILE -eq 'phone-tests'){$taskPipe+='.PhoneTests'}
elseif($env:HHUB_TEST_PROFILE -in @('mouse-port-tests','mouse-per-device-tests')){$taskPipe+='.MousePerDeviceTests'}
function Invoke-HhubRequest([string]$Operation,[hashtable]$Body=@{}) {
 $taskClient=[IO.Pipes.NamedPipeClientStream]::new('.',$taskPipe,[IO.Pipes.PipeDirection]::InOut)
 try {
  $taskClient.Connect(3000)
  $taskPayload=[Text.Encoding]::UTF8.GetBytes((@{op=$Operation;body=$Body}|ConvertTo-Json -Compress -Depth 20))
  $taskLength=[BitConverter]::GetBytes([int]$taskPayload.Length)
  $taskClient.Write($taskLength,0,4);$taskClient.Write($taskPayload,0,$taskPayload.Length);$taskClient.Flush()
  $taskReadExact={ param($taskBuffer,$taskRequired)
   $taskOffset=0
   while($taskOffset -lt $taskRequired){$taskPending=$taskClient.ReadAsync($taskBuffer,$taskOffset,$taskRequired-$taskOffset);if(-not $taskPending.Wait(5000)){throw '响应超时'};$taskCount=$taskPending.Result;if($taskCount -le 0){throw '响应被提前截断'};$taskOffset+=$taskCount}
  }
  $taskPrefix=New-Object byte[] 4;& $taskReadExact $taskPrefix 4
  $taskResponseLength=[BitConverter]::ToInt32($taskPrefix,0)
  if($taskResponseLength -le 0 -or $taskResponseLength -gt 1048576){throw '响应长度非法'}
  $taskReplyBytes=New-Object byte[] $taskResponseLength;& $taskReadExact $taskReplyBytes $taskResponseLength
  $taskClient.WriteByte(1);$taskClient.Flush()
  return [Text.Encoding]::UTF8.GetString($taskReplyBytes)|ConvertFrom-Json
 } finally {$taskClient.Dispose()}
}
$taskResults=[Collections.Generic.List[string]]::new()
$taskFirst=Invoke-HhubRequest 'snapshot'
if(-not $taskFirst.ok -or $taskFirst.state.devices.Count -ne 6){throw '设备快照不完整'}
$taskResults.Add('PASS Named-pipe response preserves the six demo configuration records')
$taskCheckCount=$taskFirst.state.checks
for($taskIndex=0;$taskIndex -lt 12;$taskIndex++){$taskReply=Invoke-HhubRequest 'snapshot';if(-not $taskReply.ok){throw '重复请求失败'}}
$taskResults.Add('PASS Twelve sequential connections receive complete JSON frames')
if($taskReply.state.checks -ne $taskCheckCount){throw '后台查询不应触发更新检查'}
$taskResults.Add('PASS Snapshot requests do not trigger update checks')
$taskBad=Invoke-HhubRequest 'invalid-test-operation'
if($taskBad.ok -or -not $taskBad.error){throw '未知操作必须返回错误'}
$taskAfter=Invoke-HhubRequest 'snapshot'
if(-not $taskAfter.ok -or $taskAfter.state.selectedDevice -ne $taskFirst.state.selectedDevice){throw '错误请求破坏状态'}
$taskResults.Add('PASS Invalid request is rejected and core remains alive')
$taskResults.Add(('Core working-set snapshot: '+[math]::Round($taskAfter.state.coreMemoryMB,1)+' MB'))
$taskResults.Add('Native-window layout and interaction are left for user manual review')
if($ReportPath){[IO.File]::WriteAllLines($ReportPath,$taskResults,(New-Object Text.UTF8Encoding($true)))}
$taskResults
