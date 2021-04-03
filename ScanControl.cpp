/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#include "ScanControl.h"
#include "Helpers.h"
#include "CmdOpts.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <vdr/plugin.h>
#include <vdr/PLUGINS/src/wirbelscan/wirbelscan_services.h>
#pragma GCC diagnostic pop

/*******************************************************************************
 * don't pollute the whole program with the namespace WIRBELSCAN_SERVICE or
 * wirbelscan_services.h
 * -> isolate it inside this object: ScanControl.o
 ******************************************************************************/
using namespace WIRBELSCAN_SERVICE;

bool StartScan(void) {
  cWirbelscanCmd cmd = { CmdStartScan, false };

  if (not wirbelscan->Service((SPlugin + std::string(SCommand)).c_str(), (void*) &cmd)) {
     ErrorMessage(__FUNCTION__ + std::string(" failed: unknown service."));
     return false;
     }
  if (not cmd.replycode)
     ErrorMessage("DoScan() failed.");
  return cmd.replycode;
}

static bool Status(cWirbelscanStatus& stat) {
  if (not wirbelscan->Service((SPlugin + std::string("Get") + SStatus).c_str(), (void*) &stat)) {
     ErrorMessage(__FUNCTION__ + std::string(" failed: unknown service."));
     return false;
     }
  return true;
}

bool Scanning(void) {
  cWirbelscanStatus stat;

  if (not Status(stat))
     return false;

  return stat.status == StatusScanning;
}

bool Import(std::vector<TChannel>& dest) {
  if (not wirbelscan->Service((SPlugin + std::string(SExport)).c_str(), (void*) &dest)) {
     ErrorMessage(__FUNCTION__ + std::string(" failed: unknown service."));
     return false;
     }
  return true;
}
