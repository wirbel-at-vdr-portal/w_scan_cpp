/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#pragma once
#include "Helpers.h"

void PrintVDR(std::vector<TChannel>& List);
void PrintVLC(std::vector<TChannel>& List, bool satip = false);
void PrintXine(std::vector<TChannel>& List, bool mplayer = false);
void PrintInitial(std::vector<TChannel>& List);
void PrintXML(std::vector<TChannel>& List);
void PrintIni(std::vector<TChannel>& List);
void PrintDat(std::vector<TChannel>& List, std::string BinaryFile);
void PrintSatellitesDat(std::vector<TChannel>& List);
