# Unmodified FxSound x64 virtual audio package

Source: fxsound2/fxsound-app, commit
d8e7a23d37ed5939c2a3090a1c1756c7f2500b17,
Installer/Drivers/Version14/win10/x64.

INF, SYS and CAT files are retained byte for byte. SHA256.json records their
hashes. Kernel signing verification (`signtool verify /kp /c ...cat ...sys`)
succeeded. Windows endpoint name and hardware ID remain FxSound Audio
Enhancer and Root\FXVAD. These are not the renamed HHUBVAD development build.

Upstream application and driver source licenses/notices are retained in the
adjacent driver LICENSE and license.rtf and vendor FxSound source notices.
When distributing this package, supply the unmodified upstream driver source
and corresponding H hub source. Do not imply original driver authorship.

Installation remains separately gated by SetupVerifyInfFile. End-to-end
installation, routing coexistence with FxSound and playback are still pending.
