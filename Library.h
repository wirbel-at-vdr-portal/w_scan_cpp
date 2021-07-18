/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#pragma once
#include "Helpers.h"

class Library {
public:
  typedef cPlugin* create_t (void);
  typedef void destroy_t(cPlugin*);

private:
  std::vector<std::string> args;
  std::vector<char*> argv;
  cPlugin* plugin;
  /* invoke plugin factory */
  void* handle;
  create_t* create;
  destroy_t* destroy;

public:
  Library(std::string FileName, std::string Arguments);
  virtual ~Library();
  cPlugin* Plugin(void);
};

extern std::vector<Library*> libs;

void UnloadLibraries(void);

bool SVDRP(cPlugin* Plugin, std::string Command, std::string& Reply);
