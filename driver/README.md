# H hub integrated virtual audio driver

Based on https://github.com/fxsound2/fxsound-driver at
`c78fc6d031d16bd0a5dbbdff4871cfb8715d343d`.
Retains upstream AGPL and Microsoft Limited Public License notices.

The adapted INF uses hardware ID `Root\HHUBVAD` and the H hub Audio
endpoint name. It must not replace or uninstall the upstream FXVAD device.
The kernel implementation remains upstream; branding changes are currently
limited to the INF. No signed binary is supplied yet.
The x64 development package now builds with successful INF verification and
catalog generation. Artifacts are in `build/audio-driver/fxvad`.
The catalog is unsigned; successful generation is not production signing.

Build requires the Windows WDK WindowsKernelModeDriver10.0 toolset.
Use `Build-Driver.ps1` for the x64 build. WDK/SDK NuGet packages at
10.0.26100.2454 have been acquired in the workspace development cache.
The current Build Tools installation also needs the Visual Studio WDK
component: merely extracting NuGet headers does not register that toolset.
The workspace cache now combines the official Build Tools component, x64
and x86 WDK tools, and Spectre libraries. Use 64-bit MSBuild to load InfVerif.
Production installation requires a matching Microsoft-signed SYS/CAT/INF
package. Changing an INF invalidates its old catalog signature. Do not enable
test signing or disable Secure Boot for end users.

Integration status:
- Installer defaults to the unmodified, kernel-signature-verified upstream
  FXVAD package in signed-upstream. The renamed HHUBVAD development build still
  requires separate production signing and is not the packaged driver.
- Native `--audio-install-integrated <inf> <report>` deployment is implemented.
  It verifies INF/catalog trust before creating a device, limits deployment to
  HHUBVAD or upstream FXVAD, updates an existing device and rolls back newly created
  devices on failure. JSON results include phase, error and reboot requirement.
  Installer handles elevation cancellation, failure and restart-required code.
  The red EQ status card now opens native install/repair when a bundled signed
  package is available. It waits off the UI thread, prevents duplicate repair
  jobs and displays cancellation, errors and reboot requirements. No repair is
  initiated merely by opening the page.
- Generic EQ auto-routing is implemented for active playback applications;
  scene and independent active application configurations take precedence.
  Generic routes stop and restore on loss of available outputs. Live playback
  verification remains outstanding; output preference is saved device, Windows
  default physical device, then the first available physical device.
- Hotplug/reconnect, output switching, clean bypass and crash route recovery.
- Live device installation succeeded without reboot. WASAPI capture passed.
  The isolated `--eq-live-probe` exercised neutral audio, real FxSound effects,
  bypass, a second physical output and two independent simultaneous playback
  processes. Both routes restored; stopping one left the other processing.
  Reports are retained in the workspace live-audio-chain/parallel JSON files.
  Controlled scene transitions also passed with actual playback: selecting a
  different scene changed the acknowledged EQ profile, and an independently
  bound background application retained its configuration across scene changes.
  Scene-test routes restored. A child playback window generated a real Windows
  foreground event handled by the production ForegroundEvent callback; the
  application path selected the higher-priority scene and acknowledged its EQ.
  The installed normal core also automatically discovered a playback test app
  and processed 36,576 frames under generic ownership. Installation, overwrite
  upgrade, driver repair and configuration preservation have passed on this PC.
  Full uninstall removed the application binaries, retained user configuration
  and shared drivers, and reinstall restored the same settings and profiles.
  Physical unplug/reconnect remains unverified because the user is currently
  unavailable to perform that test.
