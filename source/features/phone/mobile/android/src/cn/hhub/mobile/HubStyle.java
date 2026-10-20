package cn.hhub.mobile;

import android.content.Context;
import android.content.res.Configuration;
import android.content.res.ColorStateList;
import android.graphics.*;
import android.graphics.drawable.*;
import android.view.View;
import android.widget.*;

/** The desktop H hub palette and 10dp cards, adapted to native Android controls. */
final class HubStyle {
    final Context context; final int background,card,text,muted,border,control,accent;
    HubStyle(Context context){this.context=context;boolean dark=(context.getResources().getConfiguration().uiMode&Configuration.UI_MODE_NIGHT_MASK)==Configuration.UI_MODE_NIGHT_YES;
        background=Color.parseColor(dark?"#131820":"#f7f8fa");card=Color.parseColor(dark?"#1e252f":"#ffffff");text=Color.parseColor(dark?"#f5f5f5":"#1b1b1b");muted=Color.parseColor(dark?"#b7bdc5":"#60666e");border=Color.parseColor(dark?"#303b49":"#e1e6ed");control=Color.parseColor(dark?"#29313d":"#f2f4f8");accent=Color.parseColor("#0067c0");}
    int dp(float n){return Math.round(n*context.getResources().getDisplayMetrics().density);}
    GradientDrawable surface(int color,float radius,boolean outline){GradientDrawable d=new GradientDrawable();d.setColor(color);d.setCornerRadius(dp(radius));if(outline)d.setStroke(dp(1),border);return d;}
    TextView text(String value,int size,boolean secondary){TextView t=new TextView(context);t.setText(value);t.setTextSize(size);t.setTextColor(secondary?muted:text);t.setFontFeatureSettings("tnum");t.setPadding(0,dp(5),0,dp(5));return t;}
    LinearLayout card(LinearLayout parent){LinearLayout p=new LinearLayout(context);p.setOrientation(LinearLayout.VERTICAL);p.setPadding(dp(16),dp(14),dp(16),dp(14));p.setBackground(surface(card,10,true));LinearLayout.LayoutParams lp=new LinearLayout.LayoutParams(-1,-2);lp.bottomMargin=dp(12);parent.addView(p,lp);return p;}
    void button(Button b,boolean primary){b.setAllCaps(false);b.setTextSize(14);b.setTextColor(primary?Color.WHITE:text);b.setMinHeight(dp(44));b.setMinimumWidth(0);b.setPadding(dp(14),dp(6),dp(14),dp(6));b.setStateListAnimator(null);b.setElevation(0);GradientDrawable shape=surface(primary?accent:control,4,!primary);b.setBackground(new RippleDrawable(ColorStateList.valueOf(0x220067c0),shape,null));b.setAlpha(b.isEnabled()?1f:.45f);}
    void tint(SeekBar slider){slider.setProgressTintList(ColorStateList.valueOf(accent));slider.setThumbTintList(ColorStateList.valueOf(accent));slider.setProgressBackgroundTintList(ColorStateList.valueOf(border));slider.setMinimumHeight(dp(40));}
    void input(EditText input){input.setTextColor(text);input.setHintTextColor(muted);input.setTextSize(14);input.setPadding(dp(12),dp(9),dp(12),dp(9));input.setBackground(surface(card,4,true));input.setMinimumHeight(dp(44));}
    void icon(ImageView target,int type,boolean selected){target.setImageDrawable(new NavIcon(type,selected?accent:muted));}
    static final class NavIcon extends Drawable {
        final int type,color;final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
        NavIcon(int type,int color){this.type=type;this.color=color;}
        public void draw(Canvas c){Rect r=getBounds();c.save();c.translate(r.left,r.top);c.scale(r.width()/24f,r.height()/24f);paint.setColor(color);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(1.6f);paint.setStrokeCap(Paint.Cap.ROUND);
            if(type==0){c.drawRoundRect(3,4,21,16,2,2,paint);c.drawLine(12,16,12,20,paint);c.drawLine(8,20,16,20,paint);Path p=new Path();p.moveTo(6,11);p.lineTo(9,11);p.lineTo(11,7);p.lineTo(14,13);p.lineTo(16,10);p.lineTo(18,10);c.drawPath(p,paint);}
            else if(type==1){c.drawRoundRect(6,2,18,22,6,6,paint);c.drawLine(12,3,12,10,paint);c.drawLine(7,10,17,10,paint);}
            else if(type==2){for(int y=0;y<2;y++)for(int x=0;x<2;x++)c.drawRoundRect(3+x*11,3+y*11,10+x*11,10+y*11,1,1,paint);}
            else{for(int x=0;x<3;x++){float px=5+x*7;float center=x==1?15:8;c.drawLine(px,3,px,center-2,paint);c.drawLine(px,center+2,px,21,paint);c.drawCircle(px,center,2,paint);}}
            c.restore();}
        public void setAlpha(int alpha){paint.setAlpha(alpha);}public void setColorFilter(ColorFilter f){paint.setColorFilter(f);}public int getOpacity(){return PixelFormat.TRANSLUCENT;}
    }
}
