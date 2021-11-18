/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <repfunc.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include <vdr/plugin.h>
#include <vdr/diseqc.h>
#include <vdr/receiver.h>
#include <wirbelscan/common.h>
#pragma GCC diagnostic pop

#define ErrorMessage(StdString) do { std::cerr << StdString << std::endl; } while(0)
#define Message(StdString)      do { std::cerr << StdString << std::endl; } while(0)
#define OutputLine(StdString)   do { std::cout << StdString << std::endl; } while(0)

std::string VdrSource(std::string s);
bool InitCharTables(void);
