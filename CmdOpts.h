/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#pragma once
#include "Helpers.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <vdr/config.h>
#pragma GCC diagnostic pop

extern cPlugin* wirbelscan;
extern cPlugin* satip;

class wsetup {
public:
  int verbosity;
  int logFile;
  int DVB_Type;
  int DVBT_Inversion;
  int DVBC_Inversion;
  int DVBC_Symbolrate;
  int DVBC_QAM;
  int DVBC_Network_PID;
  int CountryIndex;
  int SatIndex;
  int enable_s2;
  int ATSC_type;
  uint32_t scanflags;
  int scan_remove_invalid;
  int scan_update_existing;
  int scan_append_new;
  int SignalWaitTime;
  int LockTimeout;
//std::vector<TChannel*> SingleTransponder;
//int tp_only;
  /*********************/
  std::string adapter;
  bool HelpText;
  std::string OutputFile;
  std::vector<std::string> SortCriteria;
  std::string SatipSvr;
  std::string SatipAddr;
  std::string SatipModel;
  std::string SatipDesc;
  std::string FemonChannel;
  bool ParseLCN;
};
extern wsetup WirbelscanSetup;
extern std::string OutputFormat;

bool ParseArguments(int argc, char* argv[]);
bool ParseSatipServer(std::string s);
