#pragma once
#include <Vst3/Plugin.hpp>

#include <score/plugins/application/GUIApplicationPlugin.hpp>

#include <stdexcept>
#include <ossia/detail/fmt.hpp>

namespace vst3
{

struct vst_error: public std::runtime_error
{
  template<typename... Args>
  vst_error(Args&&... args) noexcept
    : runtime_error{fmt::format(args...)}
  {

  }
};

struct ApplicationPlugin : public score::ApplicationPlugin
{
public:
  ApplicationPlugin(const score::ApplicationContext& ctx);

  void rescan();

  struct AvailablePlugin
  {
    VST3::Hosting::Module::Ptr module;
    VST3::Hosting::ClassInfo classInfo;
  };

  VST3::Hosting::Module::Ptr getModule(const std::string& path)
  {
    std::string err;
    auto it = modules.find(path);
    if(it != modules.end())
    {
      return it->second;
    }
    else
    {
      auto module = VST3::Hosting::Module::create(path, err);

      if (!module)
        throw vst_error("Failed to load VST3 ({}) : {}", path, err);

      modules[path] = module;
      return module;
    }
  }

  Steinberg::Vst::HostApplication m_host;
  ossia::string_map<VST3::Hosting::Module::Ptr> modules;
  ossia::string_map<VST3::Hosting::Module::Ptr> available;



};
}