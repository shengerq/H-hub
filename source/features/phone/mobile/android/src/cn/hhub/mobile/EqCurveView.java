package cn.hhub.mobile;

import android.content.Context;
import android.graphics.*;
import android.view.MotionEvent;
import android.view.View;
import java.util.Locale;

/** Native twelve-band curve and rotary gain controls, using H hub's palette. */
final class EqCurveView extends View {
    interface Listener {void changed(int band,double gain);}
    private final double[] gains,frequencies;
    private final int[] order,positions;
    private final HubStyle style;
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path path=new Path();
    private final Listener listener;
    private int selected=-1;private float startY;private double startGain;private boolean knob;
    EqCurveView(Context context,double[] gains,double[] frequencies,HubStyle style,Listener listener){super(context);this.gains=gains.clone();this.frequencies=frequencies.clone();this.style=style;this.listener=listener;order=new int[gains.length];positions=new int[gains.length];Integer[] indices=new Integer[gains.length];for(int i=0;i<indices.length;i++)indices[i]=i;java.util.Arrays.sort(indices,(a,b)->Double.compare(frequencies[a],frequencies[b]));for(int i=0;i<indices.length;i++){order[i]=indices[i];positions[indices[i]]=i;}setContentDescription("十二段均衡器，拖动曲线节点或频段旋钮调整增益");}
    double[] gains(){return gains.clone();}
    void setGain(int band,double gain){if(band<0||band>=gains.length)return;gains[band]=Math.max(-12,Math.min(12,Math.round(gain*2)/2.0));invalidate();}
    void reset(){for(int i=0;i<gains.length;i++)gains[i]=0;invalidate();}
    private float left(){return style.dp(30);}private float right(){return getWidth()-style.dp(30);}
    private float top(){return style.dp(30);}private float bottom(){return getHeight()-style.dp(100);}
    private float x(int band){return left()+(right()-left())*positions[band]/(gains.length-1);}
    private float y(double gain){return top()+(float)((12-gain)/24)*(bottom()-top());}
    static String frequency(double hz){return hz>=1000?String.format(Locale.ROOT,hz%1000==0?"%.0f kHz":"%.2f kHz",hz/1000):String.format(Locale.ROOT,hz%1==0?"%.0f Hz":"%.2f Hz",hz);}
    @Override protected void onDraw(Canvas canvas){super.onDraw(canvas);float zero=y(0);paint.setStrokeWidth(style.dp(1));paint.setColor(style.border);paint.setStyle(Paint.Style.STROKE);
        for(double db:new double[]{-12,-6,0,6,12}){paint.setPathEffect(db==0?new DashPathEffect(new float[]{style.dp(3),style.dp(4)},0):null);canvas.drawLine(left(),y(db),right(),y(db),paint);}paint.setPathEffect(null);
        for(int i=0;i<gains.length;i++){paint.setColor(style.border);canvas.drawLine(x(i),top(),x(i),bottom(),paint);}
        path.reset();path.moveTo(x(order[0]),zero);for(int i:order)path.lineTo(x(i),y(gains[i]));path.lineTo(x(order[gains.length-1]),zero);path.close();paint.setStyle(Paint.Style.FILL);paint.setShader(new LinearGradient(0,top(),0,bottom(),new int[]{0x550067c0,0x080067c0},null,Shader.TileMode.CLAMP));canvas.drawPath(path,paint);paint.setShader(null);
        path.reset();for(int rank=0;rank<order.length;rank++){int i=order[rank];if(rank==0)path.moveTo(x(i),y(gains[i]));else path.lineTo(x(i),y(gains[i]));}paint.setColor(style.accent);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(style.dp(2));canvas.drawPath(path,paint);
        paint.setTextAlign(Paint.Align.CENTER);paint.setTypeface(Typeface.create("sans-serif-medium",Typeface.NORMAL));
        for(int i=0;i<gains.length;i++){float px=x(i),py=y(gains[i]);paint.setStyle(Paint.Style.FILL);paint.setColor(style.card);canvas.drawCircle(px,py,style.dp(6),paint);paint.setStyle(Paint.Style.STROKE);paint.setColor(style.accent);paint.setStrokeWidth(style.dp(i==selected?3:2));canvas.drawCircle(px,py,style.dp(i==selected?8:6),paint);
            paint.setStyle(Paint.Style.FILL);paint.setColor(style.text);paint.setTextSize(style.dp(11));canvas.drawText(String.format(Locale.ROOT,"%+.1f",gains[i]),px,py-style.dp(13),paint);paint.setColor(style.muted);canvas.drawText(frequency(frequencies[i]),px,bottom()+style.dp(26),paint);
            float cy=bottom()+style.dp(57),radius=style.dp(16);RectF ring=new RectF(px-radius,cy-radius,px+radius,cy+radius);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(style.dp(4));paint.setColor(style.border);canvas.drawArc(ring,135,270,false,paint);paint.setColor(style.accent);canvas.drawArc(ring,135,(float)((gains[i]+12)/24*270),false,paint);double angle=Math.toRadians(135+(gains[i]+12)/24*270);float tx=px+(float)Math.cos(angle)*radius,ty=cy+(float)Math.sin(angle)*radius;paint.setStyle(Paint.Style.FILL);paint.setColor(style.card);canvas.drawCircle(tx,ty,style.dp(4),paint);paint.setStyle(Paint.Style.STROKE);paint.setColor(style.accent);paint.setStrokeWidth(style.dp(2));canvas.drawCircle(tx,ty,style.dp(4),paint);
        }
    }
    @Override public boolean onTouchEvent(MotionEvent event){if(!isEnabled())return false;int action=event.getActionMasked();
        if(action==MotionEvent.ACTION_DOWN){int rank=Math.round((event.getX()-left())/(right()-left())*(gains.length-1));if(rank<0||rank>=gains.length)return false;int nearest=order[rank];float px=x(nearest);boolean point=Math.abs(event.getX()-px)<style.dp(24)&&Math.abs(event.getY()-y(gains[nearest]))<style.dp(28);knob=Math.abs(event.getX()-px)<style.dp(25)&&Math.abs(event.getY()-(bottom()+style.dp(57)))<style.dp(28);if(!point&&!knob)return false;selected=nearest;startY=event.getY();startGain=gains[selected];getParent().requestDisallowInterceptTouchEvent(true);invalidate();return true;}
        if(selected<0)return false;
        if(action==MotionEvent.ACTION_MOVE){double value=startGain+(startY-event.getY())/(knob?style.dp(5):(bottom()-top())/24);value=Math.max(-12,Math.min(12,Math.round(value*2)/2.0));if(value!=gains[selected]){gains[selected]=value;listener.changed(selected,value);invalidate();}return true;}
        if(action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_CANCEL){getParent().requestDisallowInterceptTouchEvent(false);performClick();selected=-1;invalidate();return true;}return false;
    }
    @Override public boolean performClick(){super.performClick();return true;}
}
