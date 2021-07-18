/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#include "Satellites.h"
#include "Helpers.h"
#include "CmdOpts.h"

std::vector<std::string> SatIdValues;
std::vector<std::string> SatNames;

void InitSatellites(cPlugin* wirbelscan) {
  std::stringstream ss;
  std::string line;
  int code;

  ss << *wirbelscan->SVDRPCommand("LSTS", nullptr, code);
  while(std::getline(ss, line)) {
     auto items = split(line, ':');
     if (items.size() != 3) continue;
     SatIdValues.push_back  (items[1]);
     SatNames.push_back(items[2]);
     }
}

std::string SatArgs(void) {
  std::string result;
  for(auto s:SatIdValues) {
     if (result.empty())
        result += s;
     else
        result += "," + s;
     }
  return result;
}

bool PrintSatellites(void) {
  std::stringstream ss;
  for(size_t i = 0; i < SatIdValues.size(); i++)
     std::cout << FrontFill("", 8)
               << BackFill(SatIdValues[i], 20)
               << SatNames[i]
               << std::endl;
  WirbelscanSetup.HelpText = true;
  return true;
}

int GetSatelliteId(std::string IdValue) {
  for(size_t i = 0; i < SatIdValues.size(); i++)
     if (SatIdValues[i] == IdValue) return i;
  return -1;
}

std::string GetSatelliteIdValue(int Id) {
  return SatNames[Id];
}
