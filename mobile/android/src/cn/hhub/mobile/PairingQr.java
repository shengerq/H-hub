package cn.hhub.mobile;

import java.io.IOException;
import java.net.URI;
import java.net.URLDecoder;
import java.util.HashMap;
import java.util.Map;

/** Shared, strict QR parser for app links and the previous HTTPS browser QR. */
final class PairingQr {
    final String address,code,fingerprint;
    final long expires;
    private PairingQr(String address,String code,String fingerprint,long expires){this.address=address;this.code=code;this.fingerprint=fingerprint;this.expires=expires;}
    static PairingQr parse(String value,long now) throws Exception {
        if(value==null||value.length()>4096)throw new IOException("二维码内容无效");
        URI uri=new URI(value.trim());String parameters,address;
        if("hhub".equals(uri.getScheme())&&"pair".equals(uri.getHost())&&(uri.getPath()==null||uri.getPath().isEmpty())){
            if(uri.getRawFragment()!=null||uri.getUserInfo()!=null||uri.getPort()!=-1)throw new IOException("配对链接无效");
            parameters=uri.getRawQuery();address=null;
        }else if("https".equals(uri.getScheme())){
            parameters=uri.getRawFragment();address=new URI(uri.getScheme(),uri.getAuthority(),uri.getPath(),uri.getQuery(),null).toString();
        }else throw new IOException("请扫描 H hub 电脑端的配对二维码");
        Map<String,String> fields=new HashMap<>();
        if(parameters==null)throw new IOException("二维码缺少配对信息");
        for(String part:parameters.split("&")){String[] p=part.split("=",2);if(p.length!=2)throw new IOException("二维码参数无效");String key=URLDecoder.decode(p[0],"UTF-8");if(fields.put(key,URLDecoder.decode(p[1],"UTF-8"))!=null)throw new IOException("二维码包含重复参数");}
        if(address==null){if(!"2".equals(fields.get("v")))throw new IOException("二维码版本不支持");address=fields.get("address");if(!fields.containsKey("expires"))throw new IOException("二维码缺少有效期");}
        String code=fields.get("code"),pin=fields.get("fingerprint");
        if(address==null||code==null||!code.matches("[0-9]{6}")||pin==null)throw new IOException("二维码缺少地址、配对码或证书指纹");
        long expires=fields.containsKey("expires")?Long.parseLong(fields.get("expires")):0;
        if(fields.containsKey("expires")&&expires<=now)throw new IOException("二维码已过期，请在电脑更新配对码");
        HubClient validated=new HubClient(address,pin);
        return new PairingQr(validated.address,code,validated.fingerprint,expires);
    }
}
