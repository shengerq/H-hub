param([switch]$RemoveData,
 [string]$ProfileDirectory=(Join-Path $env:LOCALAPPDATA 'HhubPrototype'),
 [string]$LogPath=(Join-Path $env:TEMP 'Hhub-uninstall-data.log'))
$ErrorActionPreference='Stop'
function Write-DataLog([string]$Message) {
 [IO.File]::AppendAllText($LogPath,$Message+[Environment]::NewLine,[Text.UTF8Encoding]::new($false))
}
try {
 if(-not $RemoveData){Write-DataLog 'Keeping installed plugins and personal data';exit 0}
 $taskRoot=[IO.Path]::GetFullPath($ProfileDirectory).TrimEnd('\','/')
 if([IO.Path]::GetFileName($taskRoot) -ne 'HhubPrototype'){throw 'Unexpected profile directory; cleanup refused'}
 # Verify the absolute target, every ancestor, and every descendant before any
 # removal. A junction must never redirect cleanup outside the selected profile.
 $taskAncestor=$taskRoot
 while($taskAncestor){
  if(Test-Path -LiteralPath $taskAncestor){
   if((Get-Item -LiteralPath $taskAncestor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Profile contains a redirected path; cleanup refused'}
  }
  $taskAncestor=[IO.Path]::GetDirectoryName($taskAncestor)
 }
 if(-not (Test-Path -LiteralPath $taskRoot)){Write-DataLog 'Profile already absent';exit 0}
 function Assert-NoLinks([string]$Directory){
  foreach($taskEntry in Get-ChildItem -LiteralPath $Directory -Force){
   if($taskEntry.Attributes -band [IO.FileAttributes]::ReparsePoint){throw 'Profile contains a link; cleanup refused'}
   if($taskEntry.PSIsContainer){Assert-NoLinks $taskEntry.FullName}
  }
 }
 Assert-NoLinks $taskRoot
 Write-DataLog ('Removing user-selected profile: '+$taskRoot)
 Remove-Item -LiteralPath $taskRoot -Recurse -Force
 Write-DataLog 'Plugins and personal data removed'
} catch {
 try { Write-DataLog ('FAILED: '+$_.Exception.Message) } catch {}
 Write-Error -Message $_.Exception.Message -ErrorAction Continue
 exit 1
}
