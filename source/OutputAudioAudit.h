#pragma once
#include "EqAudio.h"
#include <endpointvolume.h>
namespace hub {
inline int microphoneStatus(std::filesystem::path const& file){try{auto e=audioEnumerator();winrt::com_ptr<IMMDeviceCollection> devices;check_hresult(e->EnumAudioEndpoints(eAll,DEVICE_STATE_ACTIVE,devices.put()));UINT count{};devices->GetCount(&count);JsonArray inputs;for(UINT i=0;i<count;i++){winrt::com_ptr<IMMDevice> d;devices->Item(i,d.put());JsonObject info;put(info,L"id",audioId(d.get()));put(info,L"name",audioName(d.get()));winrt::com_ptr<IAudioEndpointVolume> volume;check_hresult(d->Activate(__uuidof(IAudioEndpointVolume),CLSCTX_ALL,nullptr,volume.put_void()));BOOL muted{};float level{};volume->GetMute(&muted);volume->GetMasterVolumeLevelScalar(&level);put(info,L"muted",muted!=FALSE);put(info,L"level",(double)level);inputs.Append(info);}JsonObject result;result.SetNamedValue(L"inputs",inputs);write(file,std::wstring(result.Stringify()));return 0;}catch(...){return 2;}}
// Explicit diagnostic only: ten seconds of the selected render endpoint.
inline int outputAudioAudit(std::wstring const& endpoint,std::filesystem::path const& file,bool microphone=false){
 try{
  auto enumerator=audioEnumerator();winrt::com_ptr<IMMDevice> device;check_hresult(enumerator->GetDevice(endpoint.c_str(),device.put()));
  winrt::com_ptr<IAudioClient> client;check_hresult(device->Activate(__uuidof(IAudioClient),CLSCTX_ALL,nullptr,client.put_void()));
  WAVEFORMATEX format{};format.wFormatTag=WAVE_FORMAT_IEEE_FLOAT;format.nChannels=2;format.nSamplesPerSec=48000;format.wBitsPerSample=32;format.nBlockAlign=8;format.nAvgBytesPerSec=384000;
  if(microphone){auto raw=client.try_as<IAudioClient2>();if(raw){AudioClientProperties properties{};properties.cbSize=sizeof(properties);properties.eCategory=AudioCategory_Other;properties.Options=AUDCLNT_STREAMOPTIONS_RAW;raw->SetClientProperties(&properties);}}check_hresult(client->Initialize(AUDCLNT_SHAREMODE_SHARED,(microphone?0:AUDCLNT_STREAMFLAGS_LOOPBACK)|AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM|AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,0,0,&format,nullptr));
  winrt::com_ptr<IAudioCaptureClient> capture;check_hresult(client->GetService(__uuidof(IAudioCaptureClient),capture.put_void()));
  std::vector<float> samples;samples.reserve(960000);check_hresult(client->Start());auto start=GetTickCount64();
  while(samples.size()<960000&&GetTickCount64()-start<15000){UINT32 packet{};check_hresult(capture->GetNextPacketSize(&packet));while(packet){BYTE* bytes{};UINT32 count{};DWORD flags{};check_hresult(capture->GetBuffer(&bytes,&count,&flags,nullptr,nullptr));for(UINT32 i=0;i<count*2&&samples.size()<960000;i++)samples.push_back((flags&AUDCLNT_BUFFERFLAGS_SILENT)?0:reinterpret_cast<float*>(bytes)[i]);check_hresult(capture->ReleaseBuffer(count));check_hresult(capture->GetNextPacketSize(&packet));}Sleep(2);}
  client->Stop();std::ofstream stream(file,std::ios::binary|std::ios::trunc);stream.write(reinterpret_cast<char const*>(samples.data()),samples.size()*sizeof(float));return samples.size()==960000&&stream.good()?0:1;
 }catch(...){return 2;}
}
}
