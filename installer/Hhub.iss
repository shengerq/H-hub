#ifndef AppRoot
 #define AppRoot "..\release\Hhub-1.0\Hhub"
#endif
#ifndef ReleaseOutput
 #define ReleaseOutput "..\release\installers"
#endif
[Setup]
AppId={{56255994-024D-4E33-B75D-954DC37B8308}
AppName=H hub
AppVersion=1.0
AppVerName=H hub 1.0
DefaultDirName={localappdata}\Programs\Hhub
DefaultGroupName=H hub
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.22000
OutputDir={#ReleaseOutput}
OutputBaseFilename=Hhub-Setup-1.0
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
SetupIconFile={#AppRoot}\assets\hhub.ico
CloseApplications=yes
RestartApplications=no
UninstallDisplayIcon={app}\runtime\Hhub.exe
DisableProgramGroupPage=yes
SetupLogging=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "chinesesimp"; MessagesFile: "ChineseSimplified.isl"

[CustomMessages]
english.DesktopShortcut=Create a desktop shortcut
english.StartInTray=Start in the tray when signing in to Windows
english.PawnIODriver=Install PawnIO driver (CPU temperature and power; administrator permission required)
english.OpenHhub=Open H hub
english.CannotClose=Could not close H hub. Close it and try again.
english.NotClosed=H hub has not exited normally. Close it and try again.
chinesesimp.DesktopShortcut=创建桌面快捷方式
chinesesimp.StartInTray=登录 Windows 时启动到托盘
chinesesimp.PawnIODriver=安装 PawnIO 驱动（CPU 温度与功耗，需要管理员授权）
chinesesimp.OpenHhub=打开 H hub
chinesesimp.CannotClose=无法退出正在运行的 Hhub，请关闭后重试。
chinesesimp.NotClosed=Hhub 尚未正常退出，请关闭后重试。

[Tasks]
Name: "desktopicon"; Description: "{cm:DesktopShortcut}"; Flags: unchecked
Name: "startup"; Description: "{cm:StartInTray}"
Name: "pawnio"; Description: "{cm:PawnIODriver}"

[Files]
Source: "{#AppRoot}\assets\gamepad.svg"; DestDir: "{app}\assets"; Flags: ignoreversion
Source: "{#AppRoot}\assets\headset.svg"; DestDir: "{app}\assets"; Flags: ignoreversion
Source: "{#AppRoot}\assets\hhub.ico"; DestDir: "{app}\assets"; Flags: ignoreversion
Source: "{#AppRoot}\assets\hhub.png"; DestDir: "{app}\assets"; Flags: ignoreversion
Source: "{#AppRoot}\assets\keyboard.svg"; DestDir: "{app}\assets"; Flags: ignoreversion
Source: "{#AppRoot}\assets\mouse.svg"; DestDir: "{app}\assets"; Flags: ignoreversion
Source: "{#AppRoot}\assets\SOURCES.md"; DestDir: "{app}\assets"; Flags: ignoreversion
Source: "{#AppRoot}\build-info.json"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#AppRoot}\drivers\pawnio\PawnIO_setup.exe"; DestDir: "{app}\drivers\pawnio"; Flags: ignoreversion
Source: "{#AppRoot}\Hhub.Catalog.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#AppRoot}\Hhub.Core.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#AppRoot}\Hhub.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#AppRoot}\Hhub.Update.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#AppRoot}\identity\Hhub.Catalog.exe.manifest"; DestDir: "{app}\identity"; Flags: ignoreversion
Source: "{#AppRoot}\identity\Hhub.Core.exe.manifest"; DestDir: "{app}\identity"; Flags: ignoreversion
Source: "{#AppRoot}\identity\Hhub.Development.cer"; DestDir: "{app}\identity"; Flags: ignoreversion
Source: "{#AppRoot}\identity\Hhub.exe.manifest"; DestDir: "{app}\identity"; Flags: ignoreversion
Source: "{#AppRoot}\identity\Hhub.Identity.msix"; DestDir: "{app}\identity"; Flags: ignoreversion
Source: "{#AppRoot}\identity\Hhub.Sensors.exe.manifest"; DestDir: "{app}\identity"; Flags: ignoreversion
Source: "{#AppRoot}\identity\Hhub.Update.exe.manifest"; DestDir: "{app}\identity"; Flags: ignoreversion
Source: "{#AppRoot}\languages\en-US.json"; DestDir: "{app}\languages"; Flags: ignoreversion
Source: "{#AppRoot}\languages\zh-CN.json"; DestDir: "{app}\languages"; Flags: ignoreversion
Source: "{#AppRoot}\licenses\EarTrumpet-LICENSE.txt"; DestDir: "{app}\licenses"; Flags: ignoreversion
Source: "{#AppRoot}\licenses\PawnIO-GPL.txt"; DestDir: "{app}\licenses"; Flags: ignoreversion
Source: "{#AppRoot}\licenses\PawnIO-source-notice.md"; DestDir: "{app}\licenses"; Flags: ignoreversion
Source: "{#AppRoot}\Optional-plugins.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#AppRoot}\plugin-library-source.json"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\concrt140.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\CoreMessagingXP.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\dcompi.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\dwmcorei.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\DwmSceneI.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\DWriteCore.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\en-us\Microsoft.ui.xaml.dll.mui"; DestDir: "{app}\runtime\en-us"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\en-us\Microsoft.UI.Xaml.Phone.dll.mui"; DestDir: "{app}\runtime\en-us"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Hhub.PluginHost.exe"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Hhub.exe"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Hhub.runtime"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\marshal.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.DirectManipulation.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Foundation.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Graphics.Display.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Graphics.Imaging.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Graphics.Imaging.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Graphics.ImagingInternal.ImageObjectRemover.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Graphics.ImagingInternal.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Graphics.Internal.Imaging.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Graphics.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.InputStateManager.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Internal.FrameworkUdk.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Security.Authentication.OAuth.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.UI.Composition.OSSupport.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.UI.Designer.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.UI.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.UI.Input.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.UI.pri"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.UI.Text.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.UI.Windowing.Core.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.UI.Windowing.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.UI.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.UI.Xaml\Assets\map.html"; DestDir: "{app}\runtime\Microsoft.UI.Xaml\Assets"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.UI.Xaml\Assets\NoiseAsset_256X256_PNG.png"; DestDir: "{app}\runtime\Microsoft.UI.Xaml\Assets"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.UI.Xaml.Controls.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.UI.Xaml.Controls.pri"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.ui.xaml.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.UI.Xaml.Internal.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.UI.Xaml.Phone.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.ui.xaml.resources.19h1.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.ui.xaml.resources.common.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.UI.Xaml.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Web.WebView2.Core.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Web.WebView2.Core.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.AI.AICapabilities.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.AI.ContentModerationInternal.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.AI.ContentSafety.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.AI.ContentSafety.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.AI.Foundation.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.AI.FoundationInternal.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.AI.GenerativeInternal.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.AI.Imaging.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.AI.Imaging.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.AI.MachineLearning.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.AI.MachineLearning.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.AI.Text.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.AI.Text.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.AI.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.ApplicationModel.Background.UniversalBGTask.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.ApplicationModel.Background.UniversalBGTask.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.ApplicationModel.Background.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.ApplicationModel.DynamicDependency.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.ApplicationModel.Resources.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.ApplicationModel.Resources.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.ApplicationModel.WindowsAppRuntime.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.AppLifecycle.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.AppNotifications.Builder.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.AppNotifications.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.BadgeNotifications.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.Foundation.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.Globalization.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.Internal.Vision.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.Management.Deployment.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.Media.Capture.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.Private.Workloads.SessionManager.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.PrivateCommon.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.PushNotifications.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.Security.AccessControl.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.SemanticSearch.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.Storage.Pickers.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.Storage.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.System.Power.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.System.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.Vision.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.VisionInternal.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.Widgets.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.Widgets.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.Workloads.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.Workloads.Resources.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.Workloads.Resources_ec.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.Windows.Workloads.winmd"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.WindowsAppRuntime.Bootstrap.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.WindowsAppRuntime.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.WindowsAppRuntime.Insights.Resource.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\Microsoft.WindowsAppRuntime.pri"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\MRM.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\msvcp140.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\msvcp140_1.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\msvcp140_2.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\msvcp140_atomic_wait.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\msvcp140_codecvt_ids.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\NpuDetect\NPUDetect.dll"; DestDir: "{app}\runtime\NpuDetect"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\onnxruntime_providers_shared.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\PushNotificationsLongRunningTask.ProxyStub.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\resources.pri"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\RestartAgent.exe"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\SessionHandleIPCProxyStub.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\vccorlib140.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\vcruntime140.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\vcruntime140_1.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\vcruntime140_threads.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\WebView2Loader.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\WindowsAppRuntime.DeploymentExtensions.OneCore.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\WindowsAppRuntime.png"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\WindowsAppSdk.AppxDeploymentExtensions.Desktop-EventLog-Instrumentation.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\WindowsAppSdk.AppxDeploymentExtensions.Desktop.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\WinUIEdit.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\wuceffectsi.dll"; DestDir: "{app}\runtime"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\zh-CN\Microsoft.ui.xaml.dll.mui"; DestDir: "{app}\runtime\zh-CN"; Flags: ignoreversion
Source: "{#AppRoot}\runtime\zh-CN\Microsoft.UI.Xaml.Phone.dll.mui"; DestDir: "{app}\runtime\zh-CN"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\Aga.Controls.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\BlackSharp.Core.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\DiskInfoToolkit.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\Hhub.Sensors.exe"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\Hhub.Sensors.exe.config"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\HidSharp.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\LibreHardwareMonitorLib.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\LICENSE-LibreHardwareMonitor.txt"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\Microsoft.Bcl.AsyncInterfaces.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\Microsoft.Bcl.HashCode.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\Microsoft.Win32.TaskScheduler.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\monitor-sources.json"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\OxyPlot.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\OxyPlot.WindowsForms.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\RAMSPDToolkit-NDD.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\SOURCES.md"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\System.Buffers.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\System.CodeDom.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\System.Collections.Immutable.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\System.Formats.Nrbf.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\System.IO.Pipelines.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\System.Memory.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\System.Numerics.Vectors.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\System.Reflection.Metadata.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\System.Resources.Extensions.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\System.Runtime.CompilerServices.Unsafe.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\System.Security.AccessControl.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\System.Security.Principal.Windows.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\System.Text.Encodings.Web.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\System.Text.Json.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\System.Threading.AccessControl.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\System.Threading.Tasks.Extensions.dll"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\sensors\THIRD-PARTY-NOTICES.txt"; DestDir: "{app}\sensors"; Flags: ignoreversion
Source: "{#AppRoot}\SHA256SUMS.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#AppRoot}\update-source.json"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#AppRoot}\webui\home.css"; DestDir: "{app}\webui"; Flags: ignoreversion
Source: "{#AppRoot}\webui\home.js"; DestDir: "{app}\webui"; Flags: ignoreversion
Source: "{#AppRoot}\webui\index.html"; DestDir: "{app}\webui"; Flags: ignoreversion
Source: "{#AppRoot}\workloads.365.json"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#AppRoot}\workloads.j32.json"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#AppRoot}\workloads.json"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#AppRoot}\workloads.lnl.json"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#AppRoot}\workloads.qnn.json"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#AppRoot}\workloads.stx.json"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#AppRoot}\使用说明.txt"; DestDir: "{app}"; Flags: ignoreversion

[InstallDelete]
Type: files; Name: "{app}\runtime\onnxruntime.dll"
Type: files; Name: "{app}\runtime\DirectML.dll"
Type: files; Name: "{app}\Hhub.Phone.exe"
Type: files; Name: "{app}\phone-web\app.js"
Type: files; Name: "{app}\phone-web\index.html"
Type: files; Name: "{app}\phone-web\style.css"
Type: files; Name: "{app}\af-ZA\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\af-ZA\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\am-ET\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\am-ET\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\ar-SA\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\ar-SA\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\as-IN\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\as-IN\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\az-Latn-AZ\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\az-Latn-AZ\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\bg-BG\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\bg-BG\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\bn-IN\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\bn-IN\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\bs-Latn-BA\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\bs-Latn-BA\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\ca-ES\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\ca-ES\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\ca-Es-VALENCIA\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\ca-Es-VALENCIA\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\concrt140.dll"
Type: files; Name: "{app}\CoreMessagingXP.dll"
Type: files; Name: "{app}\cs-CZ\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\cs-CZ\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\cy-GB\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\cy-GB\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\da-DK\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\da-DK\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\dcompi.dll"
Type: files; Name: "{app}\de-DE\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\de-DE\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\DirectML.dll"
Type: files; Name: "{app}\dwmcorei.dll"
Type: files; Name: "{app}\DwmSceneI.dll"
Type: files; Name: "{app}\DWriteCore.dll"
Type: files; Name: "{app}\el-GR\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\el-GR\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\en-GB\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\en-GB\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\en-us\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\en-us\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\es-ES\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\es-ES\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\es-MX\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\es-MX\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\et-EE\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\et-EE\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\eu-ES\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\eu-ES\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\fa-IR\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\fa-IR\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\fi-FI\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\fi-FI\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\fil-PH\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\fil-PH\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\fr-CA\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\fr-CA\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\fr-FR\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\fr-FR\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\ga-IE\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\ga-IE\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\gd-gb\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\gd-gb\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\gl-ES\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\gl-ES\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\gu-IN\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\gu-IN\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\he-IL\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\he-IL\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\hi-IN\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\hi-IN\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\hr-HR\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\hr-HR\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\hu-HU\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\hu-HU\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\hy-AM\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\hy-AM\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\id-ID\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\id-ID\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\is-IS\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\is-IS\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\it-IT\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\it-IT\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\ja-JP\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\ja-JP\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\ka-GE\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\ka-GE\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\kk-KZ\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\kk-KZ\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\km-KH\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\km-KH\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\kn-IN\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\kn-IN\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\ko-KR\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\ko-KR\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\kok-IN\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\kok-IN\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\lb-LU\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\lb-LU\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\lo-LA\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\lo-LA\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\lt-LT\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\lt-LT\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\lv-LV\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\lv-LV\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\marshal.dll"
Type: files; Name: "{app}\mi-NZ\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\mi-NZ\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\Microsoft.DirectManipulation.dll"
Type: files; Name: "{app}\Microsoft.Foundation.winmd"
Type: files; Name: "{app}\Microsoft.Graphics.Display.dll"
Type: files; Name: "{app}\Microsoft.Graphics.Imaging.dll"
Type: files; Name: "{app}\Microsoft.Graphics.Imaging.winmd"
Type: files; Name: "{app}\Microsoft.Graphics.ImagingInternal.ImageObjectRemover.winmd"
Type: files; Name: "{app}\Microsoft.Graphics.ImagingInternal.winmd"
Type: files; Name: "{app}\Microsoft.Graphics.Internal.Imaging.winmd"
Type: files; Name: "{app}\Microsoft.Graphics.winmd"
Type: files; Name: "{app}\Microsoft.InputStateManager.dll"
Type: files; Name: "{app}\Microsoft.Internal.FrameworkUdk.dll"
Type: files; Name: "{app}\Microsoft.Security.Authentication.OAuth.winmd"
Type: files; Name: "{app}\Microsoft.UI.Composition.OSSupport.dll"
Type: files; Name: "{app}\Microsoft.UI.Designer.dll"
Type: files; Name: "{app}\Microsoft.UI.dll"
Type: files; Name: "{app}\Microsoft.UI.Input.dll"
Type: files; Name: "{app}\Microsoft.UI.pri"
Type: files; Name: "{app}\Microsoft.UI.Text.winmd"
Type: files; Name: "{app}\Microsoft.UI.Windowing.Core.dll"
Type: files; Name: "{app}\Microsoft.UI.Windowing.dll"
Type: files; Name: "{app}\Microsoft.UI.winmd"
Type: files; Name: "{app}\Microsoft.UI.Xaml\Assets\map.html"
Type: files; Name: "{app}\Microsoft.UI.Xaml\Assets\NoiseAsset_256X256_PNG.png"
Type: files; Name: "{app}\Microsoft.UI.Xaml.Controls.dll"
Type: files; Name: "{app}\Microsoft.UI.Xaml.Controls.pri"
Type: files; Name: "{app}\Microsoft.ui.xaml.dll"
Type: files; Name: "{app}\Microsoft.UI.Xaml.Internal.dll"
Type: files; Name: "{app}\Microsoft.UI.Xaml.Phone.dll"
Type: files; Name: "{app}\Microsoft.ui.xaml.resources.19h1.dll"
Type: files; Name: "{app}\Microsoft.ui.xaml.resources.common.dll"
Type: files; Name: "{app}\Microsoft.UI.Xaml.winmd"
Type: files; Name: "{app}\Microsoft.Web.WebView2.Core.dll"
Type: files; Name: "{app}\Microsoft.Web.WebView2.Core.winmd"
Type: files; Name: "{app}\Microsoft.Windows.AI.AICapabilities.dll"
Type: files; Name: "{app}\Microsoft.Windows.AI.ContentModerationInternal.winmd"
Type: files; Name: "{app}\Microsoft.Windows.AI.ContentSafety.dll"
Type: files; Name: "{app}\Microsoft.Windows.AI.ContentSafety.winmd"
Type: files; Name: "{app}\Microsoft.Windows.AI.Foundation.winmd"
Type: files; Name: "{app}\Microsoft.Windows.AI.FoundationInternal.winmd"
Type: files; Name: "{app}\Microsoft.Windows.AI.GenerativeInternal.winmd"
Type: files; Name: "{app}\Microsoft.Windows.AI.Imaging.dll"
Type: files; Name: "{app}\Microsoft.Windows.AI.Imaging.winmd"
Type: files; Name: "{app}\Microsoft.Windows.AI.MachineLearning.dll"
Type: files; Name: "{app}\Microsoft.Windows.AI.MachineLearning.winmd"
Type: files; Name: "{app}\Microsoft.Windows.AI.Text.dll"
Type: files; Name: "{app}\Microsoft.Windows.AI.Text.winmd"
Type: files; Name: "{app}\Microsoft.Windows.AI.winmd"
Type: files; Name: "{app}\Microsoft.Windows.ApplicationModel.Background.UniversalBGTask.dll"
Type: files; Name: "{app}\Microsoft.Windows.ApplicationModel.Background.UniversalBGTask.winmd"
Type: files; Name: "{app}\Microsoft.Windows.ApplicationModel.Background.winmd"
Type: files; Name: "{app}\Microsoft.Windows.ApplicationModel.DynamicDependency.winmd"
Type: files; Name: "{app}\Microsoft.Windows.ApplicationModel.Resources.dll"
Type: files; Name: "{app}\Microsoft.Windows.ApplicationModel.Resources.winmd"
Type: files; Name: "{app}\Microsoft.Windows.ApplicationModel.WindowsAppRuntime.winmd"
Type: files; Name: "{app}\Microsoft.Windows.AppLifecycle.winmd"
Type: files; Name: "{app}\Microsoft.Windows.AppNotifications.Builder.winmd"
Type: files; Name: "{app}\Microsoft.Windows.AppNotifications.winmd"
Type: files; Name: "{app}\Microsoft.Windows.BadgeNotifications.winmd"
Type: files; Name: "{app}\Microsoft.Windows.Foundation.winmd"
Type: files; Name: "{app}\Microsoft.Windows.Globalization.winmd"
Type: files; Name: "{app}\Microsoft.Windows.Internal.Vision.winmd"
Type: files; Name: "{app}\Microsoft.Windows.Management.Deployment.winmd"
Type: files; Name: "{app}\Microsoft.Windows.Media.Capture.winmd"
Type: files; Name: "{app}\Microsoft.Windows.Private.Workloads.SessionManager.winmd"
Type: files; Name: "{app}\Microsoft.Windows.PrivateCommon.winmd"
Type: files; Name: "{app}\Microsoft.Windows.PushNotifications.winmd"
Type: files; Name: "{app}\Microsoft.Windows.Security.AccessControl.winmd"
Type: files; Name: "{app}\Microsoft.Windows.SemanticSearch.winmd"
Type: files; Name: "{app}\Microsoft.Windows.Storage.Pickers.winmd"
Type: files; Name: "{app}\Microsoft.Windows.Storage.winmd"
Type: files; Name: "{app}\Microsoft.Windows.System.Power.winmd"
Type: files; Name: "{app}\Microsoft.Windows.System.winmd"
Type: files; Name: "{app}\Microsoft.Windows.Vision.winmd"
Type: files; Name: "{app}\Microsoft.Windows.VisionInternal.winmd"
Type: files; Name: "{app}\Microsoft.Windows.Widgets.dll"
Type: files; Name: "{app}\Microsoft.Windows.Widgets.winmd"
Type: files; Name: "{app}\Microsoft.Windows.Workloads.dll"
Type: files; Name: "{app}\Microsoft.Windows.Workloads.Resources.dll"
Type: files; Name: "{app}\Microsoft.Windows.Workloads.Resources_ec.dll"
Type: files; Name: "{app}\Microsoft.Windows.Workloads.winmd"
Type: files; Name: "{app}\Microsoft.WindowsAppRuntime.Bootstrap.dll"
Type: files; Name: "{app}\Microsoft.WindowsAppRuntime.dll"
Type: files; Name: "{app}\Microsoft.WindowsAppRuntime.Insights.Resource.dll"
Type: files; Name: "{app}\Microsoft.WindowsAppRuntime.pri"
Type: files; Name: "{app}\mk-MK\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\mk-MK\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\ml-IN\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\ml-IN\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\mr-IN\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\mr-IN\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\MRM.dll"
Type: files; Name: "{app}\ms-MY\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\ms-MY\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\msvcp140.dll"
Type: files; Name: "{app}\msvcp140_1.dll"
Type: files; Name: "{app}\msvcp140_2.dll"
Type: files; Name: "{app}\msvcp140_atomic_wait.dll"
Type: files; Name: "{app}\msvcp140_codecvt_ids.dll"
Type: files; Name: "{app}\mt-MT\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\mt-MT\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\nb-NO\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\nb-NO\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\ne-NP\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\ne-NP\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\nl-NL\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\nl-NL\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\nn-NO\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\nn-NO\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\NpuDetect\NPUDetect.dll"
Type: files; Name: "{app}\onnxruntime.dll"
Type: files; Name: "{app}\onnxruntime_providers_shared.dll"
Type: files; Name: "{app}\or-IN\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\or-IN\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\pa-IN\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\pa-IN\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\pl-PL\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\pl-PL\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\pt-BR\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\pt-BR\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\pt-PT\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\pt-PT\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\PushNotificationsLongRunningTask.ProxyStub.dll"
Type: files; Name: "{app}\quz-PE\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\quz-PE\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\resources.pri"
Type: files; Name: "{app}\RestartAgent.exe"
Type: files; Name: "{app}\ro-RO\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\ro-RO\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\ru-RU\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\ru-RU\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\SessionHandleIPCProxyStub.dll"
Type: files; Name: "{app}\sk-SK\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\sk-SK\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\sl-SI\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\sl-SI\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\sq-AL\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\sq-AL\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\sr-Cyrl-BA\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\sr-Cyrl-BA\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\sr-Cyrl-RS\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\sr-Cyrl-RS\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\sr-Latn-RS\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\sr-Latn-RS\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\sv-SE\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\sv-SE\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\ta-IN\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\ta-IN\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\te-IN\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\te-IN\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\th-TH\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\th-TH\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\tr-TR\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\tr-TR\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\tt-RU\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\tt-RU\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\ug-CN\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\ug-CN\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\uk-UA\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\uk-UA\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\ur-PK\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\ur-PK\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\uz-Latn-UZ\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\uz-Latn-UZ\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\vccorlib140.dll"
Type: files; Name: "{app}\vcruntime140.dll"
Type: files; Name: "{app}\vcruntime140_1.dll"
Type: files; Name: "{app}\vcruntime140_threads.dll"
Type: files; Name: "{app}\vi-VN\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\vi-VN\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\WebView2Loader.dll"
Type: files; Name: "{app}\WindowsAppRuntime.DeploymentExtensions.OneCore.dll"
Type: files; Name: "{app}\WindowsAppRuntime.png"
Type: files; Name: "{app}\WindowsAppSdk.AppxDeploymentExtensions.Desktop-EventLog-Instrumentation.dll"
Type: files; Name: "{app}\WindowsAppSdk.AppxDeploymentExtensions.Desktop.dll"
Type: files; Name: "{app}\WinUIEdit.dll"
Type: files; Name: "{app}\wuceffectsi.dll"
Type: files; Name: "{app}\zh-CN\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\zh-CN\Microsoft.UI.Xaml.Phone.dll.mui"
Type: files; Name: "{app}\zh-TW\Microsoft.ui.xaml.dll.mui"
Type: files; Name: "{app}\zh-TW\Microsoft.UI.Xaml.Phone.dll.mui"
Type: dirifempty; Name: "{app}\zh-TW"
Type: dirifempty; Name: "{app}\zh-CN"
Type: dirifempty; Name: "{app}\vi-VN"
Type: dirifempty; Name: "{app}\uz-Latn-UZ"
Type: dirifempty; Name: "{app}\ur-PK"
Type: dirifempty; Name: "{app}\uk-UA"
Type: dirifempty; Name: "{app}\ug-CN"
Type: dirifempty; Name: "{app}\tt-RU"
Type: dirifempty; Name: "{app}\tr-TR"
Type: dirifempty; Name: "{app}\th-TH"
Type: dirifempty; Name: "{app}\te-IN"
Type: dirifempty; Name: "{app}\ta-IN"
Type: dirifempty; Name: "{app}\sv-SE"
Type: dirifempty; Name: "{app}\sr-Latn-RS"
Type: dirifempty; Name: "{app}\sr-Cyrl-RS"
Type: dirifempty; Name: "{app}\sr-Cyrl-BA"
Type: dirifempty; Name: "{app}\sq-AL"
Type: dirifempty; Name: "{app}\sl-SI"
Type: dirifempty; Name: "{app}\sk-SK"
Type: dirifempty; Name: "{app}\ru-RU"
Type: dirifempty; Name: "{app}\ro-RO"
Type: dirifempty; Name: "{app}\quz-PE"
Type: dirifempty; Name: "{app}\pt-PT"
Type: dirifempty; Name: "{app}\pt-BR"
Type: dirifempty; Name: "{app}\pl-PL"
Type: dirifempty; Name: "{app}\pa-IN"
Type: dirifempty; Name: "{app}\or-IN"
Type: dirifempty; Name: "{app}\nn-NO"
Type: dirifempty; Name: "{app}\nl-NL"
Type: dirifempty; Name: "{app}\ne-NP"
Type: dirifempty; Name: "{app}\nb-NO"
Type: dirifempty; Name: "{app}\mt-MT"
Type: dirifempty; Name: "{app}\ms-MY"
Type: dirifempty; Name: "{app}\mr-IN"
Type: dirifempty; Name: "{app}\ml-IN"
Type: dirifempty; Name: "{app}\mk-MK"
Type: dirifempty; Name: "{app}\mi-NZ"
Type: dirifempty; Name: "{app}\lv-LV"
Type: dirifempty; Name: "{app}\lt-LT"
Type: dirifempty; Name: "{app}\lo-LA"
Type: dirifempty; Name: "{app}\lb-LU"
Type: dirifempty; Name: "{app}\kok-IN"
Type: dirifempty; Name: "{app}\ko-KR"
Type: dirifempty; Name: "{app}\kn-IN"
Type: dirifempty; Name: "{app}\km-KH"
Type: dirifempty; Name: "{app}\kk-KZ"
Type: dirifempty; Name: "{app}\ka-GE"
Type: dirifempty; Name: "{app}\ja-JP"
Type: dirifempty; Name: "{app}\it-IT"
Type: dirifempty; Name: "{app}\is-IS"
Type: dirifempty; Name: "{app}\id-ID"
Type: dirifempty; Name: "{app}\hy-AM"
Type: dirifempty; Name: "{app}\hu-HU"
Type: dirifempty; Name: "{app}\hr-HR"
Type: dirifempty; Name: "{app}\hi-IN"
Type: dirifempty; Name: "{app}\he-IL"
Type: dirifempty; Name: "{app}\gu-IN"
Type: dirifempty; Name: "{app}\gl-ES"
Type: dirifempty; Name: "{app}\gd-gb"
Type: dirifempty; Name: "{app}\ga-IE"
Type: dirifempty; Name: "{app}\fr-FR"
Type: dirifempty; Name: "{app}\fr-CA"
Type: dirifempty; Name: "{app}\fil-PH"
Type: dirifempty; Name: "{app}\fi-FI"
Type: dirifempty; Name: "{app}\fa-IR"
Type: dirifempty; Name: "{app}\eu-ES"
Type: dirifempty; Name: "{app}\et-EE"
Type: dirifempty; Name: "{app}\es-MX"
Type: dirifempty; Name: "{app}\es-ES"
Type: dirifempty; Name: "{app}\en-us"
Type: dirifempty; Name: "{app}\en-GB"
Type: dirifempty; Name: "{app}\el-GR"
Type: dirifempty; Name: "{app}\de-DE"
Type: dirifempty; Name: "{app}\da-DK"
Type: dirifempty; Name: "{app}\cy-GB"
Type: dirifempty; Name: "{app}\cs-CZ"
Type: dirifempty; Name: "{app}\ca-Es-VALENCIA"
Type: dirifempty; Name: "{app}\ca-ES"
Type: dirifempty; Name: "{app}\bs-Latn-BA"
Type: dirifempty; Name: "{app}\bn-IN"
Type: dirifempty; Name: "{app}\bg-BG"
Type: dirifempty; Name: "{app}\az-Latn-AZ"
Type: dirifempty; Name: "{app}\as-IN"
Type: dirifempty; Name: "{app}\ar-SA"
Type: dirifempty; Name: "{app}\am-ET"
Type: dirifempty; Name: "{app}\af-ZA"
Type: dirifempty; Name: "{app}\NpuDetect"
Type: dirifempty; Name: "{app}\Microsoft.UI.Xaml\Assets"

[Files]
Source: "Prepare-Upgrade.ps1"; Flags: dontcopy

[Icons]
Name: "{group}\H hub"; Filename: "{app}\runtime\Hhub.exe"; WorkingDir: "{app}"
Name: "{autodesktop}\H hub"; Filename: "{app}\runtime\Hhub.exe"; WorkingDir: "{app}"; Tasks: desktopicon

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "Hhub"; ValueData: """{app}\Hhub.Core.exe"""; Tasks: startup; Check: PreserveStartupChoice

[Run]
Filename: "{app}\drivers\pawnio\PawnIO_setup.exe"; Parameters: "-install -silent"; Verb: "runas"; Tasks: pawnio; Flags: shellexec waituntilterminated; Check: NeedsPawnInstall
Filename: "{app}\Hhub.Core.exe"; Flags: nowait
Filename: "{app}\runtime\Hhub.exe"; Description: "{cm:OpenHhub}"; Flags: nowait postinstall skipifsilent

[Code]
var WasExisting, StartupWasEnabled: Boolean;

function NeedsPawnInstall: Boolean;
begin
  Result := not RegKeyExists(HKLM, 'SYSTEM\CurrentControlSet\Services\PawnIO');
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
  ExtractTemporaryFile('Prepare-Upgrade.ps1');
  Command := '-NoProfile -ExecutionPolicy Bypass -File "' + ExpandConstant('{tmp}\Prepare-Upgrade.ps1') + '" -ApplicationDirectory "' + ExpandConstant('{app}') + '"';
  if not Exec(ExpandConstant('{sys}\WindowsPowerShell\v1.0\powershell.exe'), Command, '', SW_HIDE, ewWaitUntilTerminated, Code) then
    Result := CustomMessage('CannotClose')
  else if Code <> 0 then
    Result := CustomMessage('NotClosed');
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

