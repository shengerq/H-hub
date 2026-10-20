#pragma once
#include "Common.h"
#include "EqDsp.h"
#include "FxSoundEffects.h"
namespace hub {
// FxSound owns the actual EQ and five effects. The host has one output gain
// stage and one safety ceiling, so its preamp cannot change FxSound's input tone.
class EqProcessor {
 FxSoundEffects effects;double gain=1,targetGain=1;int rampLeft=0;bool enabled=false;
public:
 double inputPeak=0,outputPeak=0,attenuationDb=0;uint64_t limitedSamples=0;
 void configure(JsonObject config){
  auto bands=config.GetNamedArray(L"bands"),frequencies=config.GetNamedArray(L"frequencies");
  if(bands.Size()!=12||frequencies.Size()!=12)throw hresult_invalid_argument(L"需要 12 段参数");
  std::array<double,12> gains{},centers{};for(int i=0;i<12;i++){gains[i]=bands.GetNumberAt(i);centers[i]=frequencies.GetNumberAt(i);}
  auto active=eqActiveBands(config);auto headroomGains=gains;
  for(int i=0;i<12;i++)if(std::find(active.begin(),active.end(),i)==active.end())headroomGains[i]=0;
  enabled=flag(config,L"enabled");EqDsp response;response.configure(headroomGains,num(config,L"preamp"),flag(config,L"autoPreamp",true),enabled,0,centers);
  attenuationDb=enabled?response.preampDb:0;targetGain=std::pow(10.,attenuationDb/20.);rampLeft=960;
  effects.configure(fxEffectValues(config),enabled);effects.configureEq(gains,centers,active);
 }
 void process(float* stereo,size_t frames){
  inputPeak=outputPeak=0;if(!frames)return;if(!stereo)throw std::invalid_argument("missing audio buffer");
  for(size_t i=0;i<frames*2;i++){if(!std::isfinite(stereo[i]))stereo[i]=0;inputPeak=std::max(inputPeak,(double)std::abs(stereo[i]));}
  effects.process(stereo,frames);
  for(size_t i=0;i<frames;i++){
   if(rampLeft){gain+=(targetGain-gain)/rampLeft;--rampLeft;}
   for(int channel=0;channel<2;channel++){
    double sample=stereo[i*2+channel]*gain;if(!std::isfinite(sample))sample=0;
    if(enabled&&std::abs(sample)>.98){sample=std::clamp(sample,-.98,.98);++limitedSamples;}
    stereo[i*2+channel]=(float)sample;outputPeak=std::max(outputPeak,std::abs(sample));
   }
  }
 }
};
}
