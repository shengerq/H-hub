#ifndef AppRoot
  #define AppRoot "..\app"
#endif
#ifndef OutputDir
  #define OutputDir "..\release"
#endif
#ifndef SignedAudioRoot
  #define SignedAudioRoot "..\driver\signed-upstream"
#endif
[Setup]
AppId={{56255994-024D-4E33-B75D-954DC37B8308}
AppName=H hub
AppVersion=1.0.9
AppVerName=H hub 1.0.9
DefaultDirName={localappdata}\Programs\Hhub
DefaultGroupName=H hub
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.22000
OutputDir={#OutputDir}
OutputBaseFilename=Hhub-Setup-1.0.9
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupIconFile=..\source\assets\hhub.ico
CloseApplications=yes
RestartApplications=no
UninstallDisplayIcon={app}\Hhub.exe
DisableProgramGroupPage=yes
SetupLogging=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "chinesesimp"; MessagesFile: "ChineseSimplified.isl"

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; Flags: unchecked
Name: "startup"; Description: "登录 Windows 时启动到托盘"
Name: "audiodriver"; Description: "安装音频路由驱动（实时 EQ，需要联网和管理员授权）"
Name: "pawnio"; Description: "安装 PawnIO 驱动（CPU 温度与功耗，需要联网和管理员授权）"

[Files]
Source: "{#AppRoot}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Excludes: "*.pdb,*.ilk,*.lib,Open-Latest.ps1,Open-Latest.cmd,Hhub.AudioTest*.exe,*.hhub,developer-tools\*,plugin-ui\*,*.WebView2\*"
Source: "Install-OptionalDriver.ps1"; DestDir: "{app}\installer"; Flags: ignoreversion
Source: "..\driver\pawnio\PawnIO_setup.exe"; DestDir: "{app}\drivers\pawnio"; Flags: ignoreversion
Source: "..\driver\pawnio\COPYING"; DestDir: "{app}\licenses"; DestName: "PawnIO-GPL.txt"; Flags: ignoreversion
Source: "..\driver\pawnio\README.md"; DestDir: "{app}\licenses"; DestName: "PawnIO-source-notice.md"; Flags: ignoreversion
#ifdef SignedAudioRoot
Source: "{#SignedAudioRoot}\fxvad.inf"; DestDir: "{app}\drivers\audio"; Flags: ignoreversion
Source: "{#SignedAudioRoot}\fxvad.sys"; DestDir: "{app}\drivers\audio"; Flags: ignoreversion
Source: "{#SignedAudioRoot}\fxvadntamd64.cat"; DestDir: "{app}\drivers\audio"; Flags: ignoreversion
Source: "{#SignedAudioRoot}\README.md"; DestDir: "{app}\drivers\audio"; Flags: ignoreversion
Source: "..\driver\LICENSE"; DestDir: "{app}\licenses"; DestName: "FxSound-driver-AGPL.txt"; Flags: ignoreversion
Source: "..\driver\license.rtf"; DestDir: "{app}\licenses"; DestName: "FxSound-driver-Microsoft-license.rtf"; Flags: ignoreversion
#endif
Source: "..\RELEASE-NOTES.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\使用说明-1.0.9.md"; DestDir: "{app}"; DestName: "使用说明.md"; Flags: ignoreversion
Source: "..\source\vendor\EarTrumpet-LICENSE.txt"; DestDir: "{app}\licenses"; Flags: ignoreversion

Source: "..\source\vendor\fxsound\LICENSE-AGPL-3.0.txt"; DestDir: "{app}\licenses"; DestName: "FxSound-AGPL-3.0.txt"; Flags: ignoreversion
Source: "..\source\vendor\fxsound\README.md"; DestDir: "{app}\licenses"; DestName: "FxSound-source-notice.md"; Flags: ignoreversion

[Icons]
Name: "{group}\H hub"; Filename: "{app}\Hhub.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\H hub"; Filename: "{app}\Hhub.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "Hhub"; ValueData: """{app}\Hhub.Core.exe"""; Tasks: startup; Check: PreserveStartupChoice

[Run]
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -File ""{app}\installer\Install-OptionalDriver.ps1"" -Driver audio"; Tasks: audiodriver; Flags: waituntilterminated runhidden; Check: UseExternalAudioInstaller
Filename: "{app}\drivers\pawnio\PawnIO_setup.exe"; Verb: "runas"; Tasks: pawnio; Flags: shellexec waituntilterminated; Check: NeedsPawnInstall
Filename: "{app}\Hhub.Core.exe"; Flags: nowait
Filename: "{app}\Hhub.exe"; Description: "打开 H hub"; Flags: nowait postinstall skipifsilent

[Code]
var WasExisting, StartupWasEnabled: Boolean;

function NeedsPawnInstall: Boolean;
begin
  Result := (not WizardSilent) and (not RegKeyExists(HKLM, 'SYSTEM\CurrentControlSet\Services\PawnIO'));
end;

function UseExternalAudioInstaller: Boolean;
begin
  Result := (not WizardSilent) and (not FileExists(ExpandConstant('{app}\drivers\audio\fxvad.inf')));
end;

procedure CurStepChanged(CurStep: TSetupStep);
var DriverCode: Integer; DriverReport, DriverArgs: String;
begin
  if (CurStep = ssPostInstall) and WizardIsTaskSelected('audiodriver') and
     FileExists(ExpandConstant('{app}\drivers\audio\fxvad.inf')) then begin
    DriverReport := ExpandConstant('{localappdata}\HhubPrototype\audio-driver-install.json');
    ForceDirectories(ExtractFileDir(DriverReport));
    DriverArgs := '--audio-install-integrated "' + ExpandConstant('{app}\drivers\audio\fxvad.inf') + '" "' + DriverReport + '"';
    if not ShellExec('runas', ExpandConstant('{app}\Hhub.Core.exe'), DriverArgs, '', SW_HIDE, ewWaitUntilTerminated, DriverCode) then
      MsgBox('音频驱动安装已取消或无法启动，主程序安装继续。', mbInformation, MB_OK)
    else if DriverCode = 3010 then
      MsgBox('音频驱动已安装，请重启 Windows 后使用实时 EQ。', mbInformation, MB_OK)
    else if DriverCode <> 0 then
      MsgBox('音频驱动安装失败，详细结果保存在：' + DriverReport, mbError, MB_OK);
  end;
end;

function PreserveStartupChoice: Boolean;
begin
  Result := (not WizardSilent) or (not WasExisting) or StartupWasEnabled;
end;
function PrepareToInstall(var NeedsRestart: Boolean): String;
var Code: Integer; Command: String;
begin
  Result := '';
  WasExisting := FileExists(ExpandConstant('{app}\Hhub.Core.exe'));
  StartupWasEnabled := False;
  if RegQueryStringValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 'Hhub', Command) then
    StartupWasEnabled := Pos(Lowercase(ExpandConstant('{app}\Hhub.Core.exe')), Lowercase(Command)) > 0;
  if FileExists(ExpandConstant('{app}\Hhub.Core.exe')) then
    Exec(ExpandConstant('{app}\Hhub.Core.exe'), '--shutdown', '', SW_HIDE, ewWaitUntilTerminated, Code);
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var Code: Integer; Command: String;
begin
  if CurUninstallStep = usUninstall then begin
    Exec(ExpandConstant('{app}\Hhub.Core.exe'), '--shutdown', '', SW_HIDE, ewWaitUntilTerminated, Code);
    if RegQueryStringValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 'Hhub', Command) then
      if Command = ExpandConstant('"{app}\Hhub.Core.exe"') then
        RegDeleteValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 'Hhub');
  end;
end;

