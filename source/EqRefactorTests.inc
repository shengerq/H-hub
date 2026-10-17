 test(L"EQ render queue never fills missing frames with zeros",[&]{
  EqBuffer queue;std::array<float,10> input{.11f,.21f,.12f,.22f,.13f,.23f,.14f,.24f,.15f,.25f};queue.append(input.data(),5,false);
  std::array<float,16> output;output.fill(9.f);ensure(queue.read(output.data(),8)==5&&queue.empty());
  for(int i=0;i<10;i++)ensure(output[i]==input[i]);for(int i=10;i<16;i++)ensure(output[i]==9.f);
  ensure(queue.read(output.data(),8)==0&&output[10]==9.f);queue.append(input.data(),5,false);ensure(queue.read(output.data(),2)==2&&queue.size()==3);ensure(queue.read(output.data()+4,6)==3);
  for(int i=0;i<10;i++)ensure(output[i]==input[i]);
 });
 test(L"EQ accepts genuine silent packets without dereferencing null data",[&]{
  EqBuffer queue;queue.append(nullptr,3,true);std::array<float,8> output;output.fill(7.f);ensure(queue.read(output.data(),4)==3);
  for(int i=0;i<6;i++)ensure(output[i]==0);ensure(output[6]==7.f&&output[7]==7.f);
 });
 test(L"Host EQ matches FxSound for every retained factory tone",[&]{
  for(auto const& value:JsonArray::Parse(fxSoundPresetData)){
   auto config=object(std::wstring(value.GetObject().Stringify()));put(config,L"enabled",true);put(config,L"autoPreamp",false);put(config,L"preamp",0.0);
   EqProcessor host;host.configure(config);DfxDsp reference;ensure(reference.setSignalFormat(32,2,48000,32)==0);reference.powerOn(true);reference.eqOn(false);
   auto effects=fxEffectValues(config);for(int i=0;i<5;i++)reference.setEffectValue((DfxDsp::Effect)i,(float)effects[i]);reference.setNumBands(10);
   auto gains=config.GetNamedArray(L"bands"),frequencies=config.GetNamedArray(L"frequencies");for(int i=0;i<10;i++){reference.setEqBandFrequency(i,(float)frequencies.GetNumberAt(i+1));reference.setEqBandBoostCut(i,(float)gains.GetNumberAt(i+1));}bool eqActive=false;for(int i=1;i<=10;i++)eqActive=eqActive||gains.GetNumberAt(i)!=0;reference.eqOn(eqActive);bool processing=eqActive;for(double v:effects)processing=processing||v>0;reference.powerOn(processing);
   double error=0;for(int block=0;block<20;block++){std::array<float,960> a{},b{};for(int i=0;i<480;i++){a[i*2]=(float)(.001*sin(2*3.141592653589793*1000*(i+block*480)/48000));a[i*2+1]=(float)(.001*sin(2*3.141592653589793*6200*(i+block*480)/48000));}b=a;host.process(a.data(),480);ensure(reference.processAudio((short*)b.data(),(short*)b.data(),480,0)==0);for(int i=0;i<960;i++){ensure(std::isfinite(a[i]));error=std::max(error,(double)std::abs(a[i]-b[i]));}}ensure(error<.0001);
  }
 });
 test(L"Refactored audio remains continuous over odd sized render blocks",[&]{
  auto config=object(std::wstring(JsonArray::Parse(fxSoundPresetData).GetObjectAt(0).Stringify()));saveFxEffectValues(config,{0,0,0,0,0});put(config,L"enabled",true);put(config,L"autoPreamp",false);
  EqProcessor whole,split;whole.configure(config);split.configure(config);std::vector<float> expected(16384),actual;
  for(size_t i=0;i<8192;i++){expected[i*2]=(float)(.01+.005*sin(2*3.141592653589793*18000*i/48000));expected[i*2+1]=(float)(.01+.005*sin(2*3.141592653589793*8000*i/48000));}actual=expected;whole.process(expected.data(),8192);
  size_t offset=0;for(size_t count:{1u,127u,481u,239u,1025u,63u,2047u,4209u}){split.process(actual.data()+offset*2,count);offset+=count;}ensure(offset==8192);
  for(size_t i=0;i<actual.size();i++)ensure(std::isfinite(actual[i])&&std::abs(actual[i]-expected[i])<.0001);
 });
 test(L"EQ safety ceiling and bypass work after gain stage refactor",[&]{
  auto config=object(std::wstring(JsonArray::Parse(fxSoundPresetData).GetObjectAt(0).Stringify()));put(config,L"enabled",true);put(config,L"autoPreamp",true);
  EqProcessor processor;processor.configure(config);std::array<float,2048> samples;samples.fill(.9f);processor.process(samples.data(),1024);for(float sample:samples)ensure(std::isfinite(sample)&&std::abs(sample)<=.981f);
  EqProcessor bypass;put(config,L"enabled",false);bypass.configure(config);for(size_t i=0;i<samples.size();i++)samples[i]=(float)(.3*sin(i));auto original=samples;bypass.process(samples.data(),1024);ensure(samples==original);
 });
 test(L"Preset cleanup preserves custom presets and saved retired curves",[&]{
  auto file=report.parent_path()/L"eq-curated-migration.json";auto original=pluginFixtureModel();auto old=object(std::wstring(JsonArray::Parse(fxSoundPresetData).GetObjectAt(0).Stringify()));put(old,L"id",std::wstring(L"fps"));original.state.GetNamedArray(L"presets").Append(old);
  auto user=object(std::wstring(old.Stringify()));put(user,L"id",std::wstring(L"custom-kept"));original.state.GetNamedArray(L"presets").Append(user);
  auto config=object(std::wstring(old.Stringify()));put(config,L"preset",std::wstring(L"fps"));original.state.SetNamedValue(L"eqTemplate",config);auto bands=config.GetNamedArray(L"bands").Stringify(),effects=config.GetNamedArray(L"effects").Stringify();write(file,std::wstring(original.state.Stringify()));
  Model migrated(file,true);auto presets=migrated.state.GetNamedArray(L"presets");ensure(presets.Size()==14);find(presets,L"custom-kept");for(auto const& p:presets)ensure(str(p.GetObject(),L"id")!=L"fps");auto retained=migrated.state.GetNamedObject(L"eqTemplate");ensure(str(retained,L"preset")==L"custom"&&retained.GetNamedArray(L"bands").Stringify()==bands&&retained.GetNamedArray(L"effects").Stringify()==effects);migrated.save();Model again(file,true);ensure(again.state.GetNamedArray(L"presets").Size()==14);std::filesystem::remove(file);
 });
 test(L"Generic EQ never steals applications from another output",[&]{
  auto on=object(LR"({"activeOutputs":["headphones"]})");auto other=object(LR"({"activeOutputs":["speakers"]})");auto multiple=object(LR"({"activeOutputs":["headphones","speakers"]})");auto routed=object(LR"({"activeOutputs":["virtual"]})");
  ensure(eqApplicationUsesOutput(on,L"headphones"));ensure(!eqApplicationUsesOutput(other,L"headphones"));ensure(!eqApplicationUsesOutput(multiple,L"headphones"));ensure(!eqApplicationUsesOutput(routed,L"headphones"));ensure(eqApplicationUsesOutput(routed,L"headphones",L"virtual"));ensure(!eqApplicationUsesOutput(JsonObject{},L"headphones"));
 });
 test(L"Hhub audio node naming excludes physical output devices",[&]{ensure(EqNode::integrated(L"Speakers (FxSound Audio Enhancer)"));ensure(EqNode::integrated(L"Hhub Audio"));ensure(!EqNode::integrated(L"Speakers (Realtek Audio)"));ensure(virtualAudioName(L"Hhub Audio"));});
