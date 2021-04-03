/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#include <string>
#include <sstream>
#include "Helpers.h"
#include "DiSEqC.h"

static int pos = -1;

void DiseqcSwitchConfig(std::string Source, int LnbLo, int LnbHi, int LnbSLOF, std::string Switch) {
  size_t p = Switch.find_first_of("cu");

  if (p == std::string::npos)
     return;

  bool committed = Switch[p] == 'c';
  Switch.erase(p);

  if (Switch.empty() or (Switch.find_first_not_of("0123456789") != std::string::npos))
     return;
  pos = std::stol(Switch);

  if (committed) {
     for(int i = 4*pos; i < 4*pos + 4; i++) {
        std::stringstream ss;
        ss << BackFill(Source,7);

        if (i & 1)  ss << FrontFill("99999",6);                 else ss << FrontFill(std::to_string(LnbSLOF),6);
        if (i & 2)  ss << FrontFill("H ",3);                    else ss << FrontFill("V ",3);
        if (i & 1)  ss << FrontFill(std::to_string(LnbHi),6);   else ss << FrontFill(std::to_string(LnbLo),6);
        if (i & 2)  ss << FrontFill("t V",4);                   else ss << FrontFill(" t v",4);
        ss <<  " W15 [E0 10 38 F" << std::uppercase << std::hex << i << std::nouppercase;
        if (i & 1)  ss << "] W15 A W15 T";                      else ss << "] W15 A W15 t";

        cDiseqc* d = new cDiseqc;
        d->Parse(ss.str().c_str());
        Diseqcs.Add(d);
        }
     }
  else {
     for(int i = 0; i < 4; i++) {
        std::stringstream ss;
        ss << BackFill(Source,7);

        if (i & 1)  ss << FrontFill("99999",6);                 else ss << FrontFill(std::to_string(LnbSLOF),6);
        if (i & 2)  ss << FrontFill("H ",3);                    else ss << FrontFill("V ",3);
        if (i & 1)  ss << FrontFill(std::to_string(LnbHi),6);   else ss << FrontFill(std::to_string(LnbLo),6);
        if (i & 2)  ss << FrontFill("t V",4);                   else ss << FrontFill(" t v",4);
        ss <<  " W15 [E0 10 39 F" << std::uppercase << std::hex << pos << std::nouppercase;
        if (i & 1)  ss << "] W15 A W15 T";                      else ss << "] W15 A W15 t";

        cDiseqc* d = new cDiseqc;
        d->Parse(ss.str().c_str());
        Diseqcs.Add(d);
        }
     }
  Setup.DiSEqC = 1;
  Setup.UsePositioner = 0;
}

int DiseqcSwitchPosition(void) {
  return pos;
}

static int rotor_pos = -1;

void DiseqcRotorPosition(std::string Source, int LnbLo, int LnbHi, int LnbSLOF, int Position) {
  for(int i = 0; i < 4; i++) {
     std::stringstream ss;
     ss << BackFill(Source,7);

     if (i & 1)  ss << FrontFill("99999",6);                 else ss << FrontFill(std::to_string(LnbSLOF),6);
     if (i & 2)  ss << FrontFill("H ",3);                    else ss << FrontFill("V ",3);
     if (i & 1)  ss << FrontFill(std::to_string(LnbHi),6);   else ss << FrontFill(std::to_string(LnbLo),6);
     ss << FrontFill("t V W20 P",10) << Position << " W20 ";
     if (i & 1)  ss << "T ";                                 else ss << "t ";
     if (i & 2)  ss << "V";                                  else ss << "v";
     cDiseqc* d = new cDiseqc;
     d->Parse(ss.str().c_str());
     Diseqcs.Add(d);
     }
  Setup.DiSEqC = 1;
  Setup.UsePositioner = 1;
  rotor_pos = Position;
}

int DiseqcRotorPosition(void) {
  return rotor_pos;
}

void DiseqcRotorUsals(int LnbLo, int LnbHi, int LnbSLOF, std::string Params) {
  int item = 0;

  for(auto s:split(Params,':')) {
     if (s.empty() or (s.find_first_not_of("0123456789") != std::string::npos)) {
        std::cerr << "invalid USALS data" << std::endl;
        return;
        }

     int num = std::stol(s);
     switch(item++) {
        case 0: Setup.SiteLat         = num; break;
        case 1: Setup.SiteLon         = num; break;
        case 2: Setup.PositionerSpeed = num; break;
        case 3: Setup.PositionerSwing = num; break;
        }
     }

  for(int i = 0; i < 4; i++) {
     std::stringstream ss;
     ss << BackFill("S360E",7);

     if (i & 1)  ss << FrontFill("99999",6);                 else ss << FrontFill(std::to_string(LnbSLOF),6);
     if (i & 2)  ss << FrontFill("H ",3);                    else ss << FrontFill("V ",3);
     if (i & 1)  ss << FrontFill(std::to_string(LnbHi),6);   else ss << FrontFill(std::to_string(LnbLo),6);
     ss << " t V W20 P W20 ";
     if (i & 1)  ss << "T ";                                 else ss << "t ";
     if (i & 2)  ss << "V";                                  else ss << "v";
     cDiseqc* d = new cDiseqc;
     d->Parse(ss.str().c_str());
     Diseqcs.Add(d);
     }
  Setup.DiSEqC = 1;
  Setup.UsePositioner = 1;
}


void DiseqcScr(std::string Source, int LnbLo, int LnbHi, int LnbSLOF, std::string Params) {
  int item = 0;
  int slot = 0;
  int user_frequency = 0;
  int sat_pos = 0;
  int pin = -1;
  bool jess = false;


  for(auto s:split(Params,':')) {
     switch(item++) {
        case 0: {
           if (s.empty() or (s.find_first_not_of("0123456789") != std::string::npos)) {
              std::cerr << "invalid SCR slot" << std::endl;
              return;
              }
           slot = std::stol(s);
           break;
           }
        case 1: {
           if (s.empty() or (s.find_first_not_of("0123456789") != std::string::npos)) {
              std::cerr << "invalid SCR frequency" << std::endl;
              return;
              }
           user_frequency = std::stol(s);
           break;
           }
        case 2: {
           if      (s == "A") { sat_pos = 0;  jess = false; }
           else if (s == "B") { sat_pos = 1;  jess = false; }
           else if (s == "a") { sat_pos = 0;  jess = true;  }
           else if (s == "b") { sat_pos = 1;  jess = true;  }
           else if (s == "c") { sat_pos = 2;  jess = true;  }
           else if (s == "d") { sat_pos = 3;  jess = true;  }
           else if (s == "e") { sat_pos = 4;  jess = true;  }
           else if (s == "f") { sat_pos = 5;  jess = true;  }
           else if (s == "g") { sat_pos = 6;  jess = true;  }
           else if (s == "h") { sat_pos = 7;  jess = true;  }
           else if (s == "i") { sat_pos = 8;  jess = true;  }
           else if (s == "j") { sat_pos = 9;  jess = true;  }
           else if (s == "k") { sat_pos = 10; jess = true;  }
           else if (s == "l") { sat_pos = 11; jess = true;  }
           else if (s == "m") { sat_pos = 12; jess = true;  }
           else if (s == "n") { sat_pos = 13; jess = true;  }
           else if (s == "o") { sat_pos = 14; jess = true;  }
           else if (s == "p") { sat_pos = 15; jess = true;  }
           else {
              std::cerr << "invalid SCR sat pos" << std::endl;
              return;
              }
           break;
           }
        case 3: {
           if (s.empty() or (s.find_first_not_of("0123456789") != std::string::npos)) {
              std::cerr << "invalid SCR pin" << std::endl;
              return;
              }
           pin = std::stol(s);
           break;
           }
        }
     }

  std::stringstream ss;

  ss << BackFill(std::to_string(slot),3);
  ss << std::to_string(user_frequency);
  if (pin != -1)
     ss << FrontFill(std::to_string(pin),4);

  cScr* scr = new cScr;
  scr->Parse(ss.str().c_str());
  Scrs.Add(scr);
  ss.clear();

  for(int i = 0; i < 4; i++) {
     std::stringstream ss;
     ss << BackFill(Source,7);

     if (i & 1)  ss << FrontFill("99999",6);                 else ss << FrontFill(std::to_string(LnbSLOF),6);
     if (i & 2)  ss << FrontFill("H ",3);                    else ss << FrontFill("V ",3);
     if (i & 1)  ss << FrontFill(std::to_string(LnbHi),6);   else ss << FrontFill(std::to_string(LnbLo),6);
     ss << " t V W20 S" << BackFill(std::to_string(4*sat_pos + i),4);
     if (jess)   ss << "[70 00 00 00]";                      else ss << "[E0 10 5A 00 00]";
     ss << " W20 v";

     cDiseqc* d = new cDiseqc;
     d->Parse(ss.str().c_str());
     Diseqcs.Add(d);
     }
  Setup.DiSEqC = 1;
  Setup.UsePositioner = 0;
}


void PrintDiseqc(void) {
  if (Scrs.First() != nullptr) {
     Message("/*******************************************************************************");
     Message(" * scr");
     Message(" ******************************************************************************/");

     for(const cScr* p = Scrs.First(); p != nullptr; p = Scrs.Next(p)) {
        if (p->Pin() < 0)
           Message(BackFill(std::to_string(p->Channel()),3) + ' ' +
                   FrontFill(std::to_string(p->UserBand()),5));
        else
           Message(BackFill(std::to_string(p->Channel()),3) + ' ' +
                   FrontFill(std::to_string(p->UserBand()),5) +
                   FrontFill(std::to_string(p->Pin()),4));
        }
     }

  Message("/*******************************************************************************");
  Message(" * diseqc");
  Message(" ******************************************************************************/");

  for(const cDiseqc* p = Diseqcs.First(); p != nullptr; p = Diseqcs.Next(p)) {
     Message(BackFill(VdrSource(*cSource::ToString(p->Source())),7) +
             ' ' +
             FrontFill(std::to_string(p->Slof()),5) +
             ' ' +
             p->Polarization() +
             ' ' +
             FrontFill(std::to_string(p->Lof()),5) +
             ' ' +
             p->Commands());
     }

  Message("Setup.DiSEqC          = " + std::to_string(Setup.DiSEqC));       
  Message("Setup.UsePositioner   = " + std::to_string(Setup.UsePositioner));

  if (Setup.UsePositioner) {
     Message("Setup.SiteLat         = " + std::to_string(Setup.SiteLat));
     Message("Setup.SiteLon         = " + std::to_string(Setup.SiteLon));
     Message("Setup.PositionerSpeed = " + std::to_string(Setup.PositionerSpeed));
     Message("Setup.PositionerSwing = " + std::to_string(Setup.PositionerSwing));
     }
}
