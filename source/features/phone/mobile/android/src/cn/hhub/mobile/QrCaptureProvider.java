package cn.hhub.mobile;

import android.content.*;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;
import java.io.*;

/** Grants the system camera access to exactly one disposable app-cache photo. */
public final class QrCaptureProvider extends ContentProvider {
    static Uri uri(){return Uri.parse("content://cn.hhub.mobile.capture/capture");}
    static File file(Context context){return new File(context.getCacheDir(),"pairing-qr.jpg");}
    private void check(Uri uri){if(!uri().equals(uri))throw new SecurityException("Unknown capture URI");}
    public boolean onCreate(){return true;}
    public String getType(Uri uri){check(uri);return "image/jpeg";}
    public ParcelFileDescriptor openFile(Uri uri,String mode) throws FileNotFoundException {check(uri);if(!mode.equals("r")&&!mode.equals("w")&&!mode.equals("rw")&&!mode.equals("rwt"))throw new FileNotFoundException("Mode not allowed");return ParcelFileDescriptor.open(file(getContext()),ParcelFileDescriptor.parseMode(mode));}
    public Cursor query(Uri uri,String[] projection,String selection,String[] args,String sort){check(uri);MatrixCursor result=new MatrixCursor(new String[]{OpenableColumns.DISPLAY_NAME,OpenableColumns.SIZE});result.addRow(new Object[]{"pairing-qr.jpg",file(getContext()).length()});return result;}
    public Uri insert(Uri uri,ContentValues values){throw new UnsupportedOperationException();}
    public int delete(Uri uri,String selection,String[] args){throw new UnsupportedOperationException();}
    public int update(Uri uri,ContentValues values,String selection,String[] args){throw new UnsupportedOperationException();}
}
