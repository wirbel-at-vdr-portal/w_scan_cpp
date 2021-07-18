/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#pragma once
#include <string>
#include <vector>

extern std::vector<std::string> SatIdValues;
extern std::vector<std::string> SatNames;

class cPlugin;
void InitSatellites(cPlugin* wirbelscan);
std::string SatArgs(void);
bool PrintSatellites(void);
int GetSatelliteId(std::string IdValue);
std::string GetSatelliteIdValue(int Id);
