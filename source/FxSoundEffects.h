#pragma once
#include "vendor/fxsound/full/dsp/include/DfxDsp.h"
#include <array>
#include <vector>
#include <cmath>
#include <stdexcept>
namespace hub {
class FxSoundEffects {
 DfxDsp engine;std::array<double,5> current{};bool enabled=false,eqConfigured=false,eqActive=false;
 std::array<double,12> eqGains{},eqFrequencies{};int eqBandCount=0;std::vector<int> eqIndices;
public:
 FxSoundEffects(){if(engine.setSignalFormat(32,2,48000,32)!=0)throw std::runtime_error("FxSound signal format initialization failed");engine.eqOn(false);engine.powerOn(true);for(int i=0;i<5;i++)engine.setEffectValue((DfxDsp::Effect)i,0);}
 void configure(std::array<double,5> const& values,bool active){for(double v:values)if(!std::isfinite(v)||v<0||v>10)throw std::invalid_argument("effect range is 0 to 10");for(int i=0;i<5;i++)if(values[i]!=current[i])engine.setEffectValue((DfxDsp::Effect)i,(float)values[i]);current=values;enabled=active;}
 void configureEq(std::array<double,12> const& gains,std::array<double,12> const& frequencies,std::vector<int> indices={}){
  for(size_t i=0;i<12;i++)if(!std::isfinite(gains[i])||gains[i]<-12||gains[i]>12||!std::isfinite(frequencies[i])||frequencies[i]<20||frequencies[i]>20000)throw std::invalid_argument("invalid FxSound EQ parameters");
  if(indices.empty()){int n=(gains.front()==0&&gains.back()==0)?10:12;for(int i=0;i<n;i++)indices.push_back(i+(n==10?1:0));}int count=(int)indices.size();if(count<1||count>12)throw std::invalid_argument("invalid band count");int previous=-1;for(int i:indices){if(i<0||i>=12||i<=previous)throw std::invalid_argument("invalid band index");previous=i;}bool remapped=!eqConfigured||indices!=eqIndices;if(remapped){engine.setNumBands(count);if(engine.getNumEqBands()!=count)throw std::runtime_error("FxSound EQ band initialization failed");}
  bool active=false;for(int band=0;band<count;band++){int i=indices[band];if(remapped||eqFrequencies[i]!=frequencies[i])engine.setEqBandFrequency(band,(float)frequencies[i]);if(remapped||eqGains[i]!=gains[i])engine.setEqBandBoostCut(band,(float)gains[i]);active=active||gains[i]!=0;}eqBandCount=count;eqIndices=indices;
  eqGains=gains;eqFrequencies=frequencies;eqConfigured=true;if(active!=eqActive)engine.eqOn(active);eqActive=active;
 }
 bool processing()const {if(!enabled)return false;if(eqActive)return true;for(double v:current)if(v>0)return true;return false;}
 void process(float* stereo,size_t frames){if(!processing()||!frames)return;if(frames>32768)throw std::invalid_argument("audio block too large");if(engine.processAudio(reinterpret_cast<short*>(stereo),reinterpret_cast<short*>(stereo),(int)frames,0)!=0)throw std::runtime_error("FxSound processing failed");}
};
}

