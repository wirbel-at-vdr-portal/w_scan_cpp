/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#include "Library.h"
#include <vdr/plugin.h>

#ifdef STATIC_PLUGINS
    #include <vdr/PLUGINS/src/wirbelscan/wirbelscan.h>
    #include <vdr/PLUGINS/src/satip/satip.h>

    cPlugin* NewWirbelscanPlugin(void) {
       return new cPluginWirbelscan;
       }

    void DelWirbelscanPlugin(cPlugin* p) {
       cPluginWirbelscan* P = dynamic_cast<cPluginWirbelscan*>(p);
       if (P) delete P;
       }

    cPlugin* NewSatipPlugin(void) {
       return new cPluginSatip;
       }

    void DelSatipPlugin(cPlugin* p) {
       cPluginSatip* P = dynamic_cast<cPluginSatip*>(p);
       if (P) delete P;
       }
  
    Library::create_t* GetPluginCreator(std::string FileName) {
      if (FileName.find("libvdr-wirbelscan") != std::string::npos)
         return &NewWirbelscanPlugin;
      if (FileName.find("libvdr-satip") != std::string::npos)
         return &NewSatipPlugin;
      return nullptr;
    }

    Library::destroy_t* GetPluginDestroyer(std::string FileName) {
      if (FileName.find("libvdr-wirbelscan") != std::string::npos)
         return &DelWirbelscanPlugin;
      if (FileName.find("libvdr-satip") != std::string::npos)
         return &DelSatipPlugin;
      return nullptr;
    }
#else
   #include <dlfcn.h>
#endif


std::vector<Library*> libs;

Library::Library(std::string FileName, std::string Arguments)
  : plugin(nullptr), handle(nullptr) {
  #ifdef STATIC_PLUGINS
     (void) handle;
     create  = GetPluginCreator(FileName);
     destroy = GetPluginDestroyer(FileName);
  #else
     handle  = dlopen(FileName.c_str(), RTLD_NOW);
     create  = (create_t*) dlsym(handle, "VDRPluginCreator");
     if (create == nullptr)  {
        ErrorMessage("Error while loading " + FileName);
        ErrorMessage(dlerror());
        return;
        }

     destroy = (destroy_t*) dlsym(handle, "VDRPluginDestroyer");
     if (destroy == nullptr) {
        ErrorMessage(dlerror());
        return;
        }
  #endif


  args = split(Arguments, ';');
  args.insert(args.begin(), FileName);
  for(auto s:args)
     argv.push_back((char*) s.c_str());

  plugin = create();
  plugin->ProcessArgs(argv.size(), argv.data());
}

Library::~Library() {
  #ifndef STATIC_PLUGINS
     if (handle == nullptr)
        return;
  #endif

  if (destroy != nullptr)
     destroy(plugin);

  #ifndef STATIC_PLUGINS     
     dlclose(handle);
  #endif
  }

cPlugin* Library::Plugin(void) {
  return plugin;
}

void UnloadLibraries(void) {
  for(auto l:libs)
     delete(l);

  libs.clear();
}

bool SVDRP(cPlugin* Plugin, std::string Command, std::string& Reply) {
  int replyCode = 900;
  cString s = Plugin->SVDRPCommand(Command.c_str(), 0, replyCode);
  Reply = *s;
  return (replyCode >= 900) and (replyCode <= 999);
}
