/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#pragma once
#include <string>
#include <vector>

extern int CountryId;
extern std::vector<std::string> CountryIdValues;
extern std::vector<std::string> CountryNames;

class cPlugin;
void InitCountries(cPlugin* wirbelscan);
std::string CountryArgs(void);
bool PrintCountries(void);
int GetCountryId(std::string IdValue);
