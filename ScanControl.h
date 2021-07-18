/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#pragma once
#include "Helpers.h"

bool StartScan(void);
bool Scanning(void);
bool Import(std::vector<TChannel>& dest);
