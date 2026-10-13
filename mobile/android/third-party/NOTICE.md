Android 0.2.0 uses ZXing core 3.5.4 for offline QR image decoding, under Apache License 2.0.

Upstream: https://github.com/zxing/zxing/tree/zxing-3.5.4

Artifact: https://repo.maven.apache.org/maven2/com/google/zxing/core/3.5.4/core-3.5.4.jar

SHA-256: 71de5d89341b5fcf5dd89da7f44e84d825d0e084cdf3ec77c9abe26b0f0ceb13

The complete license is in ZXing-LICENSE.txt and in the APK's assets/ZXing-LICENSE.txt.

The app calls installed scanners with the ZXing scan Intent. If no compatible scanner is available, the Android system camera captures a photo and the app decodes it on a worker thread. No custom camera preview or WebView is used. No Google Play services requirement is introduced.
