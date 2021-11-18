/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#include <sstream>
#include "Countries.h"
#include "Helpers.h"
#include "CmdOpts.h"

int CountryId = 0;
std::vector<std::string> CountryIdValues;
std::vector<std::string> CountryNames;

void InitCountries(cPlugin* wirbelscan) {
  std::stringstream ss;
  std::string line;
  int code;

  ss << *wirbelscan->SVDRPCommand("LSTC", nullptr, code);
  while(std::getline(ss, line)) {
     auto items = SplitStr(line, ':');
     if (items.size() != 3) continue;
     CountryIdValues.push_back  (items[1]);
     CountryNames.push_back(items[2]);
     }
}

std::string CountryArgs(void) {
  std::string result;
  for(auto s:CountryIdValues) {
     if (result.empty())
        result += s;
     else
        result += "," + s;
     }
  return result;
}

bool PrintCountries(void) {
  std::stringstream ss;
  for(size_t i = 0; i < CountryIdValues.size(); i++) 
     std::cout << "        "
               << CountryIdValues[i]
               << "              "
               << CountryNames[i]
               << std::endl;
  WirbelscanSetup.HelpText = true;
  return true;
}

int GetCountryId(std::string IdValue) {
  for(size_t i = 0; i < CountryIdValues.size(); i++)
     if (CountryIdValues[i] == IdValue) return i;
  return -1;
}
