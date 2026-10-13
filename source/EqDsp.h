#pragma once
#include <array>
#include <cmath>
#include <complex>
#include <algorithm>
#include <stdexcept>
#include <cstdint>
#include "vendor/fxsound/FxSoundFilters.h"

namespace hub {
struct EqDsp {
 static constexpr std::array<double,12> frequencies={31.25,62.5,115.734,214.311,396.85,734.867,1360.79,2519.84,4666.12,8640.48,16000,20000};
 struct Coeff {double b0=1,b1=0,b2=0,a1=0,a2=0;};
 struct Filter {Coeff current,target;double z1[2]{},z2[2]{};};
 std::array<Filter,12> filters;double rate=48000,gain=1,targetGain=1,wet=0,targetWet=0;int rampLeft=0;double preampDb=0;uint64_t clipped=0;double inputPeak=0,outputPeak=0;
 static constexpr wchar_t const* engineName=L"FxSound 派生 Graphic EQ";
 explicit EqDsp(double sampleRate=48000):rate(sampleRate){if(!std::isfinite(rate)||rate<40000||rate>192000)throw std::invalid_argument("unsupported sample rate");}
 static void validate(std::array<double,12> const& bands,double preamp){for(double v:bands)if(!std::isfinite(v)||v<-12||v>12)throw std::invalid_argument("EQ band range is -12 to 12 dB");if(!std::isfinite(preamp)||preamp<-24||preamp>0)throw std::invalid_argument("preamp range is -24 to 0 dB");}
 // FxSound coefficient design; keep the host's twelve bands and fixed bandwidth.
 static Coeff peak(double frequency,double db,double rate){
  if(!std::isfinite(frequency)||!std::isfinite(db)||!std::isfinite(rate)||rate<=0)throw std::invalid_argument("invalid filter parameters");
  if(db==0||frequency>=rate/2)return {};
  fxsound::filt2ndOrderBoostCutShelfFilterType f;f.r_samp_freq=rate;f.r_center_freq=frequency;f.boost=db;f.Q=1.4142135623730951;
  fxsound::filtCalcParametric(&f);Coeff c{f.b0,f.b1,f.b2,f.a1,f.a2};
  if(!std::isfinite(c.b0)||!std::isfinite(c.b1)||!std::isfinite(c.b2)||!std::isfinite(c.a1)||!std::isfinite(c.a2)||std::abs(c.a2)>=1||1+c.a1+c.a2<=0||1-c.a1+c.a2<=0)throw std::invalid_argument("unstable EQ filter");
  return c;
 }
 static double response(Coeff const& c,double frequency,double rate){auto z=std::exp(std::complex<double>(0,-2*3.14159265358979323846*frequency/rate));return std::abs((c.b0+c.b1*z+c.b2*z*z)/(1.0+c.a1*z+c.a2*z*z));}
 double responseDb(double frequency,bool includePreamp=true)const{double magnitude=1;for(auto const& f:filters)magnitude*=response(f.target,frequency,rate);return 20*std::log10(std::max(1e-12,magnitude))+(includePreamp?preampDb:0);}
 static void validateFrequencies(std::array<double,12> const& centers){for(double f:centers)if(!std::isfinite(f)||f<20||f>20000)throw std::invalid_argument("frequency range is 20 to 20000 Hz");}
 void configure(std::array<double,12> const& bands,double preamp,bool automatic,bool enabled,int rampSamples=0,std::array<double,12> const& centers=frequencies){
  validate(bands,preamp);validateFrequencies(centers);if(rampSamples<0)throw std::invalid_argument("invalid ramp length");
  std::array<Coeff,12> designed;for(size_t i=0;i<12;i++)designed[i]=peak(centers[i],bands[i],rate);
  auto dbAt=[&](double frequency){double magnitude=1;for(auto const& c:designed)magnitude*=response(c,frequency,rate);return 20*std::log10(std::max(1e-12,magnitude));};
  double highest=0;for(int i=0;i<2048;i++)highest=std::max(highest,dbAt(20*std::pow(std::min(20000.0,rate*0.499)/20,i/2047.0)));for(double f:centers)if(f<rate/2)highest=std::max(highest,dbAt(f));
  for(size_t i=0;i<12;i++)filters[i].target=designed[i];preampDb=preamp-(automatic&&highest>0.01?highest+0.5:0);targetGain=std::pow(10.0,preampDb/20);targetWet=enabled?1:0;rampLeft=rampSamples;if(!rampLeft){gain=targetGain;wet=targetWet;for(auto& f:filters)f.current=f.target;}
 }
 static void approach(double& a,double b,double fraction){a+=(b-a)*fraction;}
 void frame(float& left,float& right){if(rampLeft){double fraction=1.0/rampLeft;approach(gain,targetGain,fraction);approach(wet,targetWet,fraction);for(auto& f:filters){approach(f.current.b0,f.target.b0,fraction);approach(f.current.b1,f.target.b1,fraction);approach(f.current.b2,f.target.b2,fraction);approach(f.current.a1,f.target.a1,fraction);approach(f.current.a2,f.target.a2,fraction);}--rampLeft;}float* channels[2]={&left,&right};for(int ch=0;ch<2;ch++){double dry=std::isfinite(*channels[ch])?*channels[ch]:0,input=dry;inputPeak=std::max(inputPeak,std::abs(dry));for(auto& f:filters){auto c=f.current;double output=c.b0*input+f.z1[ch];f.z1[ch]=(input-output)*c.b1+f.z2[ch];f.z2[ch]=c.b2*input-c.a2*output;if(!std::isfinite(output)||!std::isfinite(f.z1[ch])||!std::isfinite(f.z2[ch])){output=0;f.z1[ch]=f.z2[ch]=0;}input=output;if(std::abs(f.z1[ch])<1e-30)f.z1[ch]=0;if(std::abs(f.z2[ch])<1e-30)f.z2[ch]=0;}double out=dry*(1-wet)+input*gain*wet;if(wet>0&&std::abs(out)>0.98){out=std::clamp(out,-0.98,0.98);++clipped;}outputPeak=std::max(outputPeak,std::abs(out));*channels[ch]=(float)out;}}
 void reset(){for(auto& f:filters){std::fill(std::begin(f.z1),std::end(f.z1),0);std::fill(std::begin(f.z2),std::end(f.z2),0);}clipped=0;inputPeak=outputPeak=0;}
};
}
