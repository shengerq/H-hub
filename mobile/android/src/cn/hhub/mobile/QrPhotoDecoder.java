package cn.hhub.mobile;

import android.content.Context;
import android.graphics.*;
import android.net.Uri;
import com.google.zxing.*;
import com.google.zxing.common.HybridBinarizer;
import java.io.*;
import java.util.*;

final class QrPhotoDecoder {
    static String decode(Context context,Uri photo) throws Exception {
        BitmapFactory.Options options=new BitmapFactory.Options();options.inJustDecodeBounds=true;
        try(InputStream in=context.getContentResolver().openInputStream(photo)){BitmapFactory.decodeStream(in,null,options);}
        if(options.outWidth<=0||options.outHeight<=0||options.outWidth>30000||options.outHeight>30000)throw new IOException("无法读取二维码图片");
        options.inSampleSize=1;while(Math.max(options.outWidth,options.outHeight)/options.inSampleSize>1400)options.inSampleSize*=2;
        options.inJustDecodeBounds=false;options.inPreferredConfig=Bitmap.Config.ARGB_8888;
        Bitmap bitmap;try(InputStream in=context.getContentResolver().openInputStream(photo)){bitmap=BitmapFactory.decodeStream(in,null,options);}
        if(bitmap==null)throw new IOException("图片解码失败");
        Map<DecodeHintType,Object> hints=new EnumMap<>(DecodeHintType.class);hints.put(DecodeHintType.POSSIBLE_FORMATS,Collections.singletonList(BarcodeFormat.QR_CODE));hints.put(DecodeHintType.TRY_HARDER,Boolean.TRUE);
        try {
            int width=bitmap.getWidth(),height=bitmap.getHeight();int[] pixels=new int[width*height];bitmap.getPixels(pixels,0,width,0,0,width,height);
            RGBLuminanceSource source=new RGBLuminanceSource(width,height,pixels);
            for(int invert=0;invert<2;invert++)try{return new MultiFormatReader().decode(new BinaryBitmap(new HybridBinarizer(invert==0?source:source.invert())),hints).getText();}catch(NotFoundException ignored){}
        }finally{bitmap.recycle();}
        throw new IOException("没有识别到二维码，请靠近电脑屏幕并对焦后重试");
    }
}
