#pragma once
#include <array>
#include <deque>
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace hub {
// A render request consumes captured frames only. Empty space is never padded
// with synthetic silence; WASAPI's SILENT packets are legitimate captured data.
class EqBuffer {
 std::deque<std::array<float,2>> frames;
public:
 size_t size()const{return frames.size();}bool empty()const{return frames.empty();}
 void append(float const* pcm,size_t count,bool silent){
  if(!silent&&count&&!pcm)throw std::invalid_argument("missing captured samples");
  for(size_t i=0;i<count;i++)frames.push_back(silent?std::array<float,2>{0,0}:std::array<float,2>{std::isfinite(pcm[i*2])?pcm[i*2]:0.f,std::isfinite(pcm[i*2+1])?pcm[i*2+1]:0.f});
 }
 size_t read(float* destination,size_t capacity){
  auto count=std::min(capacity,size());if(count&&!destination)throw std::invalid_argument("missing render buffer");
  for(size_t i=0;i<count;i++){auto frame=frames.front();frames.pop_front();destination[i*2]=frame[0];destination[i*2+1]=frame[1];}return count;
 }
};
}
