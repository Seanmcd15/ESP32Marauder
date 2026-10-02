#pragma once

template <typename SettingsType, typename FileSystemType>
bool ensureCurrentSettings(SettingsType& settings, FileSystemType& file_system) {
  const char* legacy_type = settings.getSettingType("ChanHop");
  if (legacy_type == nullptr || legacy_type[0] == '\0') {
    return settings.createDefaultSettings(file_system);
  }

  const char* string_settings[] = {"wu", "wt", "wdg_key"};
  for (const char* name : string_settings) {
    const char* type = settings.getSettingType(name);
    if (type == nullptr || type[0] == '\0') {
      return settings.ensureStringSettings(string_settings, 3);
    }
  }

  return true;
}
