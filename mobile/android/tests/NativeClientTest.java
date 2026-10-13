package cn.hhub.mobile;

import java.io.*;
import java.nio.charset.StandardCharsets;
import org.json.JSONObject;

/** Runs the exact APK network client against the desktop service, without a handset. */
public final class NativeClientTest {
    public static void main(String[] args) throws Exception {
        JSONObject setup=new JSONObject(new BufferedReader(new InputStreamReader(System.in,StandardCharsets.UTF_8)).readLine());
        String address=setup.getString("address"),pin=setup.getString("fingerprint");
        String qr=setup.getString("pairingUri");long now=System.currentTimeMillis()/1000;
        PairingQr pairing=PairingQr.parse(qr,now);
        if(!pairing.address.equals(address)||!pairing.fingerprint.equals(pin)||pairing.expires<=now)throw new AssertionError("Desktop QR metadata mismatch");
        com.google.zxing.common.BitMatrix matrix=new com.google.zxing.qrcode.QRCodeWriter().encode(qr,com.google.zxing.BarcodeFormat.QR_CODE,600,600);
        int[] pixels=new int[600*600];for(int y=0;y<600;y++)for(int x=0;x<600;x++)pixels[y*600+x]=matrix.get(x,y)?0xFF000000:0xFFFFFFFF;
        String decoded=new com.google.zxing.MultiFormatReader().decode(new com.google.zxing.BinaryBitmap(new com.google.zxing.common.HybridBinarizer(new com.google.zxing.RGBLuminanceSource(600,600,pixels)))).getText();
        if(!decoded.equals(qr))throw new AssertionError("QR decoder round-trip");
        PairingQr.parse(address+"/#code="+pairing.code+"&fingerprint="+pin,now);
        rejectQr(qr+"&code=123456",now);
        rejectQr(qr.replace("expires="+pairing.expires,"expires=0"),now);
        rejectQr(qr.replace("v=2","v=99"),now);
        rejectQr(qr.replace(java.net.URLEncoder.encode(address+"/","UTF-8"),java.net.URLEncoder.encode("https://8.8.8.8/","UTF-8")),now);
        System.out.println("PASS Desktop QR round-trip, legacy QR import, duplicate/expired/unsupported/public QR rejection");
        HubClient client=new HubClient(address,pin);client.token=setup.getString("token");
        JSONObject status=client.request("/api/status",null);
        if(!status.getBoolean("ok")||status.getBoolean("control")||!status.getJSONObject("data").has("monitor"))throw new AssertionError("Native status response");
        System.out.println("PASS APK HubClient reads desktop telemetry over pinned TLS with Bearer authorization");
        try{client.request("/api/action",new JSONObject().put("kind","deviceVibrate").put("id","missing"));throw new AssertionError("Read-only action accepted");}
        catch(HubClient.ApiException e){if(e.status!=403)throw e;}
        System.out.println("PASS Native read-only connection cannot invoke hardware controls");
        HubClient unauthenticated=new HubClient(address,pin);
        try{unauthenticated.request("/api/status",null);throw new AssertionError("Missing token accepted");}
        catch(HubClient.ApiException e){if(e.status!=401)throw e;}
        System.out.println("PASS Native connection without authorization is rejected");
        String wrong=(pin.startsWith("0")?"1":"0")+pin.substring(1);
        HubClient mismatch=new HubClient(address,wrong);mismatch.token=client.token;
        try{mismatch.request("/api/status",null);throw new AssertionError("Wrong certificate pin accepted");}
        catch(javax.net.ssl.SSLHandshakeException expected){}
        System.out.println("PASS APK HubClient rejects a mismatched certificate fingerprint");
        try{new HubClient("http://192.168.1.2:43827",pin);throw new AssertionError("Cleartext accepted");}catch(IOException expected){}
        try{new HubClient("https://8.8.8.8",pin);throw new AssertionError("Public endpoint accepted");}catch(IOException expected){}
        System.out.println("PASS APK HubClient refuses cleartext and public endpoints");
    }
    private static void rejectQr(String value,long now) throws Exception {try{PairingQr.parse(value,now);throw new AssertionError("Invalid QR accepted");}catch(java.io.IOException|IllegalArgumentException expected){}}
}
