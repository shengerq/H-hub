#pragma once
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>
#include <utility>

namespace hub {
struct EqCurve {
 using Bands=std::array<double,12>;
 static void order(Bands& centers,Bands& gains){
  std::array<std::pair<double,double>,12> values;
  for(size_t i=0;i<values.size();i++)values[i]={centers[i],gains[i]};
  std::stable_sort(values.begin(),values.end(),[](auto a,auto b){return a.first<b.first;});
  for(size_t i=0;i<values.size();i++){centers[i]=values[i].first;gains[i]=values[i].second;}
 }
 static std::pair<double,double> bounds(size_t i,Bands const& centers,double minimumRatio=1){
  double low=i?std::max(centers[i-1]+0.1,centers[i-1]*minimumRatio):20,high=i+1<centers.size()?std::min(centers[i+1]-0.1,centers[i+1]/minimumRatio):20000;
  // Keep existing coincident bands intact; editing one must never move its neighbours.
  if(low>high){low=i?centers[i-1]:20;high=i+1<centers.size()?centers[i+1]:20000;}
  // Loading or selecting an existing tight pair must not silently change its frequency.
  low=std::min(low,centers[i]);high=std::max(high,centers[i]);
  return {low,high};
 }
 static double clampFrequency(size_t i,double desired,Bands const& centers,double minimumRatio=1){
  auto [low,high]=bounds(i,centers,minimumRatio);return std::clamp(desired,low,high);
 }
 // The solid edit guide passes through every control point. The DSP response is plotted separately.
 static std::vector<std::pair<double,double>> guide(Bands const& centers,Bands const& gains){
  std::vector<std::pair<double,double>> points;
  if(centers.front()>20)points.emplace_back(20,gains.front());
  points.emplace_back(centers.front(),gains.front());
  for(size_t i=0;i+1<centers.size();i++)for(int k=1;k<=16;k++){
   double t=k/16.0,s=t*t*(3-2*t);
   double frequency=k==16?centers[i+1]:std::exp(std::log(centers[i])*(1-t)+std::log(centers[i+1])*t);
   points.emplace_back(frequency,gains[i]*(1-s)+gains[i+1]*s);
  }
  if(centers.back()<20000)points.emplace_back(20000,gains.back());
  return points;
 }
};
}
