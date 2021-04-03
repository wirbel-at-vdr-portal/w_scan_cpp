/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#pragma once
#include <string>

void DiseqcSwitchConfig(std::string Source, int LnbLo, int LnbHi, int LnbSLOF, std::string Switch);
int  DiseqcSwitchPosition(void);
void DiseqcRotorPosition(std::string Source, int LnbLo, int LnbHi, int LnbSLOF, int Position);
int  DiseqcRotorPosition(void);
void DiseqcRotorUsals(int LnbLo, int LnbHi, int LnbSLOF, std::string Params);
void DiseqcScr(std::string Source, int LnbLo, int LnbHi, int LnbSLOF, std::string Params);
void PrintDiseqc(void);
