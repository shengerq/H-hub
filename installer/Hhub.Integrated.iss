#ifndef AppRoot
 #define AppRoot "..\..\..\legacy-installer-stage\Hhub"
#endif
#ifndef ReleaseOutput
 #define ReleaseOutput "..\..\..\交付\Hhub-旧版三功能集成与内存修复版"
#endif
[Setup]
AppId={{56255994-024D-4E33-B75D-954DC37B8308}
AppName=H hub
AppVersion=1.0
AppVerName=H hub 1.0 集成功能版
DefaultDirName={localappdata}\Programs\Hhub
DefaultGroupName=H hub
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.22000
OutputDir={#ReleaseOutput}
OutputBaseFilename=Hhub-Setup-1.0-Integrated-EQ-r6
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupIconFile={#AppRoot}\assets\hhub.ico
UninstallDisplayIcon={app}\Hhub.exe
CloseApplications=yes
RestartApplications=no
DisableProgramGroupPage=yes
SetupLogging=yes
; Re-evaluate default tasks on upgrades instead of inheriting an earlier skipped EQ task.
UsePreviousTasks=no

[Languages]
Name: "chinesesimp"; MessagesFile: "ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[CustomMessages]
chinesesimp.EQDriver=安装或修复 EQ 音频驱动，并将显示名称设为 H hub Audio（需要管理员授权）
chinesesimp.PawnIO=安装系统监测 PawnIO 驱动（需要管理员授权）
chinesesimp.Desktop=创建桌面快捷方式
chinesesimp.Startup=登录 Windows 时启动到托盘
chinesesimp.Open=打开 H hub
chinesesimp.DriverFailure=EQ 驱动安装或改名未完成。请查看日志：%1。是否重试？
english.EQDriver=Install or repair the EQ audio driver and name it H hub Audio (administrator approval required)
english.PawnIO=Install the PawnIO monitoring driver (administrator approval required)
english.Desktop=Create a desktop shortcut
english.Startup=Start in the tray when signing in to Windows
english.Open=Open H hub
english.DriverFailure=EQ driver installation or naming did not complete. See %1. Retry?

[Tasks]
Name: "eqdriver"; Description: "{cm:EQDriver}"
Name: "pawnio"; Description: "{cm:PawnIO}"; Flags: unchecked
Name: "desktopicon"; Description: "{cm:Desktop}"; Flags: unchecked
Name: "startup"; Description: "{cm:Startup}"; Flags: unchecked

[Files]
Source: "{#AppRoot}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "Prepare-Upgrade.ps1"; Flags: dontcopy
Source: "Prepare-Upgrade.ps1"; DestDir: "{app}\installer"; DestName: "Stop-Hhub.ps1"; Flags: ignoreversion
Source: "Remove-UserData.ps1"; DestDir: "{app}\installer"; Flags: ignoreversion
Source: "Verify-AudioDriverReport.ps1"; Flags: dontcopy

[InstallDelete]
Type: filesandordirs; Name: "{app}\runtime"
Type: files; Name: "{app}\Hhub.Core.exe"
Type: files; Name: "{app}\sensors\Hhub.Sensors.exe"
Type: files; Name: "{app}\sensors\Hhub.Sensors.exe.config"
Type: files; Name: "{app}\Optional-plugins.md"
Type: files; Name: "{app}\使用说明.txt"

[Icons]
Name: "{group}\H hub"; Filename: "{app}\Hhub.exe"; WorkingDir: "{app}"; AppUserModelID: "Hhub.Desktop"
Name: "{autodesktop}\H hub"; Filename: "{app}\Hhub.exe"; WorkingDir: "{app}"; AppUserModelID: "Hhub.Desktop"; Check: UpdateDesktopShortcut

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "Hhub"; ValueData: """{app}\core\Hhub.exe"""; Check: UpdateStartupEntry; Flags: uninsdeletevalue

[Run]
Filename: "{app}\drivers\pawnio\PawnIO_setup.exe"; Parameters: "-install -silent"; Verb: "runas"; Tasks: pawnio; Flags: shellexec waituntilterminated
Filename: "{app}\Hhub.exe"; Description: "{cm:Open}"; Flags: nowait postinstall skipifsilent

[Code]
var AudioRestart, KeepUserData: Boolean;

function UpdateDesktopShortcut(): Boolean;
begin
  Result := WizardIsTaskSelected('desktopicon') or FileExists(ExpandConstant('{autodesktop}\H hub.lnk'));
end;

function UpdateStartupEntry(): Boolean;
var Existing: String;
begin
  Result := WizardIsTaskSelected('startup');
  if RegQueryStringValue(HKCU, 'Software\Microsoft\Windows\CurrentVersion\Run', 'Hhub', Existing) then
    Result := Result or (CompareText(Existing, ExpandConstant('"{app}\Hhub.Core.exe"')) = 0) or
      (CompareText(Existing, ExpandConstant('"{app}\core\Hhub.exe"')) = 0);
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var Code: Integer; Command, PreparationLog: String;
begin
  Result := '';
  ExtractTemporaryFile('Prepare-Upgrade.ps1');
  PreparationLog := ExpandConstant('{localappdata}\HhubPrototype\logs\installer-prepare.log');
  Command := '-NoProfile -ExecutionPolicy Bypass -File "' + ExpandConstant('{tmp}\Prepare-Upgrade.ps1') + '" -ApplicationDirectory "' + ExpandConstant('{app}') + '" -LogPath "' + PreparationLog + '"';
  if not Exec(ExpandConstant('{sys}\WindowsPowerShell\v1.0\powershell.exe'), Command, '', SW_HIDE, ewWaitUntilTerminated, Code) then
    Result := '无法正常关闭旧版 H hub。'
  else if Code <> 0 then
    Result := '旧版退出或配置迁移失败。详细原因见：' + PreparationLog;
  Log('Profile preparation exit code: ' + IntToStr(Code) + '; details: ' + PreparationLog);
end;

procedure CurStepChanged(Step: TSetupStep);
var Code, ReportCode, Choice: Integer; Exe, ReportPath, VerifyCommand: String; Done: Boolean;
begin
  if (Step = ssPostInstall) and WizardIsTaskSelected('eqdriver') then begin
    Exe := ExpandConstant('{app}\features\eq\HhubAudioDriverSetup.exe');
    ReportPath := ExpandConstant('{localappdata}\HhubPrototype\audio-driver-setup.json');
    ExtractTemporaryFile('Verify-AudioDriverReport.ps1');
    VerifyCommand := '-NoProfile -ExecutionPolicy Bypass -File "' + ExpandConstant('{tmp}\Verify-AudioDriverReport.ps1') + '" -ReportPath "' + ReportPath + '"';
    Done := False;
    repeat
      Code := -1;
      if FileExists(ReportPath) then DeleteFile(ReportPath);
      Done := False;
      if ShellExec('runas', Exe, '--from-setup "' + ReportPath + '"', ExpandConstant('{app}'), SW_HIDE, ewWaitUntilTerminated, Code) then
        Done := (Code = 0) or (Code = 3010);
      if Done then begin
        ReportCode := -1;
        Done := Exec(ExpandConstant('{sys}\WindowsPowerShell\v1.0\powershell.exe'), VerifyCommand, '', SW_HIDE, ewWaitUntilTerminated, ReportCode);
        Done := Done and (ReportCode = 0);
        Log('EQ driver report verification exit code: ' + IntToStr(ReportCode));
      end;
      if not Done then begin
        Choice := MsgBox(FmtMessage(CustomMessage('DriverFailure'), [ReportPath]), mbError, MB_RETRYCANCEL);
        if Choice <> IDRETRY then RaiseException('EQ 驱动安装或改名未完成。');
      end;
    until Done;
    AudioRestart := Code = 3010;
    Log('EQ driver installer completed with exit code ' + IntToStr(Code));
  end;
end;

function NeedRestart(): Boolean;
begin
  Result := AudioRestart;
end;

function InitializeUninstall(): Boolean;
var Dialog: TSetupForm; KeepBox: TNewCheckBox; Explanation: TNewStaticText;
    ConfirmButton, CancelButton: TNewButton;
begin
  KeepUserData := True;
  Result := True;
  if UninstallSilent then Exit;
  Dialog := CreateCustomForm(ScaleX(510), ScaleY(225), False, True);
  try
    Dialog.Caption := '卸载 H hub';
    Dialog.ClientWidth := ScaleX(510);
    Dialog.ClientHeight := ScaleY(225);
    Dialog.Position := poScreenCenter;
    KeepBox := TNewCheckBox.Create(Dialog);
    KeepBox.Parent := Dialog;
    KeepBox.SetBounds(ScaleX(22), ScaleY(25), ScaleX(465), ScaleY(30));
    KeepBox.Caption := '保留已安装插件和个人数据（推荐）';
    KeepBox.Checked := True;
    Explanation := TNewStaticText.Create(Dialog);
    Explanation.Parent := Dialog;
    Explanation.SetBounds(ScaleX(22), ScaleY(70), ScaleX(465), ScaleY(86));
    Explanation.AutoSize := False;
    Explanation.WordWrap := True;
    Explanation.Caption := '保留后，重新安装可继续使用插件、设置、场景、EQ 预设和手机配对。' #13#10 #13#10 '取消勾选将清除这些内容。共享音频驱动保持安装。';
    ConfirmButton := TNewButton.Create(Dialog);
    ConfirmButton.Parent := Dialog;
    ConfirmButton.SetBounds(ScaleX(285), ScaleY(175), ScaleX(95), ScaleY(30));
    ConfirmButton.Caption := '继续卸载';
    ConfirmButton.Default := True;
    ConfirmButton.ModalResult := mrOk;
    CancelButton := TNewButton.Create(Dialog);
    CancelButton.Parent := Dialog;
    CancelButton.SetBounds(ScaleX(390), ScaleY(175), ScaleX(95), ScaleY(30));
    CancelButton.Caption := '取消';
    CancelButton.Cancel := True;
    CancelButton.ModalResult := mrCancel;
    Result := Dialog.ShowModal = mrOk;
    if Result then begin
      KeepUserData := KeepBox.Checked;
      if not KeepUserData then
        Result := MsgBox('确定清除全部已安装插件和个人数据？清除后无法恢复。', mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES;
    end;
  finally
    Dialog.Free;
  end;
end;

procedure CurUninstallStepChanged(Step: TUninstallStep);
var Code: Integer; Command, PowerShell, DetailLog: String;
begin
  if Step = usUninstall then begin
    PowerShell := ExpandConstant('{sys}\WindowsPowerShell\v1.0\powershell.exe');
    DetailLog := ExpandConstant('{tmp}\Hhub-uninstall.log');
    Command := '-NoProfile -ExecutionPolicy Bypass -File "' + ExpandConstant('{app}\installer\Stop-Hhub.ps1') + '" -StopOnly -LogPath "' + DetailLog + '"';
    if not Exec(PowerShell, Command, '', SW_HIDE, ewWaitUntilTerminated, Code) then
      RaiseException('无法关闭 H hub，卸载已停止。');
    if Code <> 0 then RaiseException('关闭 H hub 失败，卸载已停止。日志：' + DetailLog);
    if not KeepUserData then begin
      Command := '-NoProfile -ExecutionPolicy Bypass -File "' + ExpandConstant('{app}\installer\Remove-UserData.ps1') + '" -RemoveData -LogPath "' + DetailLog + '"';
      if not Exec(PowerShell, Command, '', SW_HIDE, ewWaitUntilTerminated, Code) then
        RaiseException('无法启动数据清理，卸载已停止。');
      if Code <> 0 then RaiseException('数据清理未完成，卸载已停止。日志：' + DetailLog);
    end;
  end;
end;

