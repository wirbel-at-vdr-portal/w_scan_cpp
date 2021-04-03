/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#include "Helpers.h"
#include "Lnb.h"

bool SetLnb(std::string lnb) {
  std::stringstream ss;
  if (lnb == "?") {
     ss << "UNIVERSAL  Europe          Dual LO   9750/10600/11700 (10800 .. 12700)" << std::endl
        << "DBS        North America   Single LO 11250            (12200 .. 12700)" << std::endl
        << "STANDARD   ---             Single LO 10000            (10945 .. 11450)" << std::endl
        << "ENHANCED   Europe          Single LO 9750             (10700 .. 11700)" << std::endl
        << "C-BAND     ---             Single LO 5150             ( 3700 ..  4200)" << std::endl
        << "C-MULTI    ---             Dual LO   5150/5750        ( 3700 ..  4200)" << std::endl
        << "AUSTRALIA  Australia       Single LO 10700            (11700 .. 12750)" << std::endl
        << "low[,high[,switch]]" << std::endl;
     return false;
     }
  else if (lnb == "UNIVERSAL") { Setup.LnbFrequLo =  9750; Setup.LnbFrequHi = 10600; Setup.LnbSLOF = 11700; }
  else if (lnb == "DBS")       { Setup.LnbFrequLo = 11250; Setup.LnbFrequHi =     0; Setup.LnbSLOF =     0; }
  else if (lnb == "STANDARD")  { Setup.LnbFrequLo = 10000; Setup.LnbFrequHi =     0; Setup.LnbSLOF =     0; }
  else if (lnb == "ENHANCED")  { Setup.LnbFrequLo =  9750; Setup.LnbFrequHi =     0; Setup.LnbSLOF =     0; }
  else if (lnb == "C-BAND")    { Setup.LnbFrequLo =  5150; Setup.LnbFrequHi =     0; Setup.LnbSLOF =     0; }
  else if (lnb == "C-MULTI")   { Setup.LnbFrequLo =  5150; Setup.LnbFrequHi =  5750; Setup.LnbSLOF =     0; }
  else if (lnb == "AUSTRALIA") { Setup.LnbFrequLo = 10700; Setup.LnbFrequHi =     0; Setup.LnbSLOF =     0; }
  else {
     if (lnb.find_first_not_of("0123456789,") != std::string::npos) return false;
     Setup.LnbFrequLo = 0;
     Setup.LnbFrequHi = 0;
     Setup.LnbSLOF    = 0;
     auto it = split(lnb, ',');
     for(size_t i = 0; i < it.size(); i++) {
        switch(i) {
           case 0: Setup.LnbFrequLo = std::stol(it[i]); break;
           case 1: Setup.LnbFrequHi = std::stol(it[i]); break;
           case 2: Setup.LnbSLOF    = std::stol(it[i]); break;
           }
        }
     }
  return true;
}

void GetLnb(int& Low, int& High, int& Switch) {
  Low    = Setup.LnbFrequLo;
  High   = Setup.LnbFrequHi;
  Switch = Setup.LnbSLOF;
}

void PrintLnb(void) {
  Message("/*******************************************************************************");
  Message(" * lnb");
  Message(" ******************************************************************************/");

  Message("Setup.LnbFrequLo      = " + std::to_string(Setup.LnbFrequLo)); 
  Message("Setup.LnbFrequHi      = " + std::to_string(Setup.LnbFrequHi));
  Message("Setup.LnbSLOF         = " + std::to_string(Setup.LnbSLOF));
}
