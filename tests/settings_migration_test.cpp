#include "../esp32_marauder/settings_migration.h"

#include <initializer_list>
#include <iostream>
#include <string>
#include <unordered_map>

struct FakeFileSystem {};

struct FakeSettings {
  std::unordered_map<std::string, std::string> settings;
  std::unordered_map<std::string, std::string> values;
  bool reset = false;
  bool fail_add = false;
  int batch_add_calls = 0;

  FakeSettings(
      std::initializer_list<std::pair<const std::string, std::string>> initial)
      : settings(initial) {}

  const char* getSettingType(const char* key) {
    const auto found = settings.find(key);
    return found == settings.end() ? "" : found->second.c_str();
  }

  bool createDefaultSettings(FakeFileSystem&) {
    reset = true;
    settings = {
        {"ChanHop", "bool"},
        {"wu", "String"},
        {"wt", "String"},
        {"wdg_key", "String"},
    };
    return true;
  }

  bool ensureStringSettings(const char* const* names, int count) {
    ++batch_add_calls;
    if (fail_add) return false;
    for (int i = 0; i < count; ++i) {
      settings[names[i]] = "String";
    }
    return true;
  }
};

bool expect(bool condition, const char* message) {
  if (!condition) std::cerr << message << '\n';
  return condition;
}

int main() {
  FakeFileSystem fs;

  FakeSettings legacy{{
      {"ChanHop", "bool"},
      {"ClientSSID", "String"},
      {"ClientPW", "String"},
  }};
  legacy.values["ClientSSID"] = "field-unit";
  if (!expect(ensureCurrentSettings(legacy, fs), "legacy migration failed") ||
      !expect(!legacy.reset, "legacy settings were reset") ||
      !expect(legacy.values["ClientSSID"] == "field-unit", "legacy values were not preserved") ||
      !expect(legacy.batch_add_calls == 1, "new settings were not added in one write") ||
      !expect(legacy.settings["wu"] == "String", "wu was not added") ||
      !expect(legacy.settings["wt"] == "String", "wt was not added") ||
      !expect(legacy.settings["wdg_key"] == "String", "wdg_key was not added")) {
    return 1;
  }

  FakeSettings current{{
      {"ChanHop", "bool"},
      {"wu", "String"},
      {"wt", "String"},
      {"wdg_key", "String"},
  }};
  const auto current_count = current.settings.size();
  if (!expect(ensureCurrentSettings(current, fs), "current schema was rejected") ||
      !expect(!current.reset, "current settings were reset") ||
      !expect(current.batch_add_calls == 0, "current schema was rewritten") ||
      !expect(current.settings.size() == current_count, "current schema was modified")) {
    return 1;
  }

  FakeSettings unsupported{{{"ClientSSID", "String"}}};
  if (!expect(ensureCurrentSettings(unsupported, fs), "unsupported schema reset failed") ||
      !expect(unsupported.reset, "unsupported schema was not reset")) {
    return 1;
  }

  FakeSettings write_failure{{{"ChanHop", "bool"}}};
  write_failure.fail_add = true;
  if (!expect(!ensureCurrentSettings(write_failure, fs), "migration ignored a write failure") ||
      !expect(!write_failure.reset, "write failure reset legacy settings")) {
    return 1;
  }

  return 0;
}
