/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#pragma once

bool SetLnb(std::string lnb);
void GetLnb(int& Low, int& High, int& Switch);
void PrintLnb(void);
