package cn.hhub.mobile;

import org.json.JSONObject;
import java.io.*;
import java.net.*;
import java.nio.charset.StandardCharsets;
import java.security.*;
import java.security.cert.*;
import javax.net.ssl.*;

/** One pinned PC connection. Never installs a process-wide trust override. */
public final class HubClient {
    final String address, fingerprint;
    private final SSLSocketFactory sockets;
    volatile String token = "";

    public HubClient(String address, String fingerprint) throws Exception {
        URI uri = new URI(address.trim());
        String host = uri.getHost();
        if (!"https".equals(uri.getScheme()) || host == null || uri.getUserInfo() != null
                || uri.getQuery() != null || uri.getFragment() != null
                || !(uri.getPath().isEmpty() || uri.getPath().equals("/")))
            throw new IOException("请输入电脑端显示的 HTTPS 地址，不包含路径");
        String[] octets = host.split("\\.");
        if (octets.length != 4) throw new IOException("请使用电脑端显示的局域网 IPv4 地址");
        int[] ip = new int[4];
        for (int i = 0; i < 4; i++) { ip[i] = Integer.parseInt(octets[i]); if(ip[i]<0||ip[i]>255)throw new IOException("地址无效"); }
        if (!(ip[0]==10 || ip[0]==192&&ip[1]==168 || ip[0]==172&&ip[1]>=16&&ip[1]<=31 || ip[0]==127))
            throw new IOException("仅支持可信局域网地址");
        this.address = "https://" + host + (uri.getPort() < 0 ? "" : ":" + uri.getPort());
        this.fingerprint = fingerprint.replaceAll("[\\s:-]", "").toUpperCase(java.util.Locale.ROOT);
        if (!this.fingerprint.matches("[0-9A-F]{64}")) throw new IOException("请输入电脑端显示的完整 SHA-256 证书指纹");
        final String pin = this.fingerprint;
        TrustManager[] trust = {new X509TrustManager() {
            public X509Certificate[] getAcceptedIssuers() { return new X509Certificate[0]; }
            public void checkClientTrusted(X509Certificate[] c, String a) throws CertificateException { throw new CertificateException(); }
            public void checkServerTrusted(X509Certificate[] c, String a) throws CertificateException {
                try {
                    if(c==null||c.length==0)throw new CertificateException("电脑未提供证书");
                    c[0].checkValidity();
                    byte[] hash = MessageDigest.getInstance("SHA-256").digest(c[0].getEncoded());
                    StringBuilder hex = new StringBuilder(); for(byte b:hash)hex.append(String.format("%02X",b&255));
                    if(!MessageDigest.isEqual(pin.getBytes(StandardCharsets.US_ASCII),hex.toString().getBytes(StandardCharsets.US_ASCII)))
                        throw new CertificateException("证书指纹不一致，请在电脑端重新核对");
                } catch(CertificateException e){throw e;} catch(Exception e){throw new CertificateException(e);}
            }
        }};
        SSLContext context = SSLContext.getInstance("TLS"); context.init(null, trust, new SecureRandom()); sockets = context.getSocketFactory();
    }

    public JSONObject request(String path, JSONObject body) throws Exception {
        if(!path.equals("/api/pair")&&!path.equals("/api/pair-status")&&!path.equals("/api/status")&&!path.equals("/api/action"))throw new IOException("接口无效");
        HttpsURLConnection connection = (HttpsURLConnection)new URL(address+path).openConnection();
        connection.setSSLSocketFactory(sockets);
        // The leaf certificate pin is the identity; the endpoint host must still match this connection.
        String expectedHost = new URL(address).getHost();
        connection.setHostnameVerifier((host,session)->host.equals(expectedHost));
        connection.setInstanceFollowRedirects(false); connection.setConnectTimeout(5000); connection.setReadTimeout(7000);
        connection.setRequestProperty("Accept","application/json");
        if(!token.isEmpty())connection.setRequestProperty("Authorization","Bearer "+token);
        try {
            if(body!=null){connection.setRequestMethod("POST");connection.setDoOutput(true);connection.setRequestProperty("Content-Type","application/json");
                byte[] bytes=body.toString().getBytes(StandardCharsets.UTF_8);connection.setFixedLengthStreamingMode(bytes.length);
                try(OutputStream out=connection.getOutputStream()){out.write(bytes);}}
            int status=connection.getResponseCode();InputStream stream=status>=400?connection.getErrorStream():connection.getInputStream();
            if(stream==null)throw new IOException("电脑返回空响应");
            ByteArrayOutputStream out=new ByteArrayOutputStream();try(InputStream in=stream){byte[] buffer=new byte[4096];int n;while((n=in.read(buffer))!=-1){if(out.size()+n>1048576)throw new IOException("响应超出限制");out.write(buffer,0,n);}}
            JSONObject data=new JSONObject(out.toString("UTF-8"));
            if(status<200||status>=300||!data.optBoolean("ok",true))throw new ApiException(status,data.optString("error","电脑请求失败"));
            return data;
        } finally {connection.disconnect();}
    }
    public static final class ApiException extends IOException { final int status; ApiException(int status,String message){super(message);this.status=status;} }
}
