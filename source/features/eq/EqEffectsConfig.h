#pragma once
#include <array>
#include <vector>
namespace hub {
inline std::pair<double,double> fxFrequencyRange(int count,int band,double current){
 if(count<1||count>12||band<0||band>=count)throw hresult_invalid_argument();
 if(count==1)return {current,current};
 double first=(count==10||count==5)?62.5:20,last=(count==10||count==5)?16000:20000,ratio=last/first;
 double low=band==0?first:std::round(first*std::pow(ratio,(band*2.0-1)/(count*2.0-2)));
 if(band>0)low+=low<1000?1:10;
 double high=band==count-1?last:std::round(first*std::pow(ratio,(band*2.0+1)/(count*2.0-2)));
 return {low,high};
}
inline std::vector<int> eqActiveBands(JsonObject config){
 std::vector<int> result;
 if(config.HasKey(L"activeBands")){auto list=config.GetNamedArray(L"activeBands");if(!list.Size()||list.Size()>12)throw hresult_invalid_argument(L"频段数量为 1–12");int last=-1;for(auto const& v:list){double n=v.GetNumber();int i=(int)n;if(n!=i||i<0||i>=12||i<=last)throw hresult_invalid_argument(L"频段索引无效");result.push_back(i);last=i;}}
 else {auto gains=config.GetNamedArray(L"bands",JsonArray{});bool expanded=gains.Size()==12&&(gains.GetNumberAt(0)!=0||gains.GetNumberAt(11)!=0);for(int i=expanded?0:1;i<(expanded?12:11);i++)result.push_back(i);}
 return result;
}
inline std::array<double,5> fxEffectValues(JsonObject config){std::array<double,5> values{};if(!config.HasKey(L"effects"))return values;auto a=config.GetNamedArray(L"effects");if(a.Size()!=5)throw hresult_invalid_argument(L"需要 5 个增强参数");for(int i=0;i<5;i++){values[i]=a.GetNumberAt(i);if(!std::isfinite(values[i])||values[i]<0||values[i]>10)throw hresult_invalid_argument(L"增强效果范围为 0–10");}return values;}
inline void saveFxEffectValues(JsonObject config,std::array<double,5> const& values){JsonArray a;for(double v:values)a.Append(JsonValue::CreateNumberValue(v));config.SetNamedValue(L"effects",a);}
}
