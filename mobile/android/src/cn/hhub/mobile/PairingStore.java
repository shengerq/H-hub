package cn.hhub.mobile;

import android.content.Context;
import android.security.keystore.KeyGenParameterSpec;
import android.security.keystore.KeyProperties;
import android.util.Base64;
import java.security.KeyStore;
import javax.crypto.*;
import javax.crypto.spec.GCMParameterSpec;
import org.json.JSONObject;

final class PairingStore {
    private final Context context;
    PairingStore(Context context){this.context=context;}
    private javax.crypto.SecretKey key() throws Exception {
        KeyStore store=KeyStore.getInstance("AndroidKeyStore");store.load(null);
        if(!store.containsAlias("hhub.pairing")){KeyGenerator generator=KeyGenerator.getInstance(KeyProperties.KEY_ALGORITHM_AES,"AndroidKeyStore");
            generator.init(new KeyGenParameterSpec.Builder("hhub.pairing",KeyProperties.PURPOSE_ENCRYPT|KeyProperties.PURPOSE_DECRYPT).setBlockModes(KeyProperties.BLOCK_MODE_GCM).setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE).build());generator.generateKey();}
        return (javax.crypto.SecretKey)store.getKey("hhub.pairing",null);
    }
    void save(HubClient client) throws Exception {
        JSONObject data=new JSONObject().put("address",client.address).put("fingerprint",client.fingerprint).put("token",client.token);
        Cipher cipher=Cipher.getInstance("AES/GCM/NoPadding");cipher.init(Cipher.ENCRYPT_MODE,key());byte[] encrypted=cipher.doFinal(data.toString().getBytes("UTF-8"));
        if(!context.getSharedPreferences("pairing",Context.MODE_PRIVATE).edit().putString("iv",Base64.encodeToString(cipher.getIV(),Base64.NO_WRAP)).putString("data",Base64.encodeToString(encrypted,Base64.NO_WRAP)).commit())throw new java.io.IOException("无法保存授权");
    }
    HubClient load() throws Exception {
        android.content.SharedPreferences prefs=context.getSharedPreferences("pairing",Context.MODE_PRIVATE);String data=prefs.getString("data","");if(data.isEmpty())return null;
        Cipher cipher=Cipher.getInstance("AES/GCM/NoPadding");cipher.init(Cipher.DECRYPT_MODE,key(),new GCMParameterSpec(128,Base64.decode(prefs.getString("iv",""),Base64.NO_WRAP)));
        JSONObject saved=new JSONObject(new String(cipher.doFinal(Base64.decode(data,Base64.NO_WRAP)),"UTF-8"));HubClient client=new HubClient(saved.getString("address"),saved.getString("fingerprint"));client.token=saved.getString("token");return client;
    }
    void clear(){context.getSharedPreferences("pairing",Context.MODE_PRIVATE).edit().clear().apply();}
}
