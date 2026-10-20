 void migrateEqPresets(){
  auto previous=state.GetNamedArray(L"presets",JsonArray{});auto factory=JsonArray::Parse(fxSoundPresetData);JsonArray curated;
  auto supported=[&](std::wstring const& id){for(auto const& p:factory)if(str(p.GetObject(),L"id")==id)return true;return false;};
  auto retired=[&](std::wstring const& id){return id==L"flat"||id==L"fps"||id==L"music";};
  // Saved application curves are snapshots. Retiring a factory menu entry must
  // never replace their bands, frequencies, effects or headroom settings.
  auto retainCurve=[&](JsonObject config){auto id=str(config,L"preset");if(retired(id)){
   for(auto const& item:previous){auto preset=item.GetObject();if(str(preset,L"id")!=id)continue;
    for(auto key:{L"bands",L"frequencies",L"effects",L"activeBands",L"preamp",L"autoPreamp"})if(!config.HasKey(key)&&preset.HasKey(key))config.SetNamedValue(key,preset.GetNamedValue(key));break;
   }put(config,L"preset",std::wstring(L"custom"));
  }};
  for(auto key:{L"eq",L"eqRealPrefs",L"audioProfiles"})for(auto const& value:state.GetNamedArray(key,JsonArray{}))retainCurve(value.GetObject());
  if(state.HasKey(L"eqTemplate"))retainCurve(state.GetNamedObject(L"eqTemplate"));
  for(auto const& value:factory)curated.Append(value);
  for(auto const& value:previous){auto id=str(value.GetObject(),L"id");if(!supported(id)&&!retired(id))curated.Append(value);}
  state.SetNamedValue(L"presets",curated);
 }
