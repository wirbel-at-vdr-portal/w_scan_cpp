/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#include <chrono>
#include "Helpers.h"
#include "CmdOpts.h"
#include "Satellites.h"
#include "Countries.h"
#include "Library.h"
#include "Lnb.h"
#include "DiSEqC.h"
#include "ScanControl.h"
#include "OutputFormats.h"
#include "Femon.h"
#include "Version.h"

int main(int argc, char* argv[]) {
  Message("/*******************************************************************************");
  Message(" * w_scan_cpp Version " + version);
  Message(" ******************************************************************************/\n");

  if (not(ParseArguments(argc, argv))) {
     ErrorMessage("Error reading command line arguments. Exit!");
     return -1;
     }

  if (WirbelscanSetup.HelpText) {
     UnloadLibraries();
     return 0;
     }

  InitCharTables();

  for(auto l:libs) {
     l->Plugin()->Initialize();
     l->Plugin()->Start();
     }

  Message("/*******************************************************************************");
  Message(" * devices");
  Message(" ******************************************************************************/");
  if (satip == nullptr) {
     if (WirbelscanSetup.adapter.empty()) {
        cDvbDevice::Initialize();
        }
     else {
        if (WirbelscanSetup.adapter.find("/dev/dvb/adapter") == 0) {
           std::string s(WirbelscanSetup.adapter);
           s.erase(0, std::string("/dev/dvb/adapter").size());
           int a = std::stoi(s.substr(0, s.find("frontend")-1));
           s.erase(0, s.find("frontend") + std::string("frontend").size());
           int f = std::stoi(s);
           new cDvbDevice(a, f);
           }
        else if (WirbelscanSetup.adapter.find_first_of("0123456789") == 0) {
           int a = std::stoi(WirbelscanSetup.adapter);
           new cDvbDevice(a, 0);
           }
        }
     }
  else
     Message("skip DVB devices - using satip plugin.");

  while (!cDevice::WaitForAllDevicesReady(3));
  if (satip) {
     std::string Reply;
     for(int i=0; (i < 5) and not SVDRP(satip, "LIST", Reply); i++) {
        SVDRP(satip, "SCAN", Reply);
        Message(Reply);
        mSleep(3000);
        }
     for(auto line:SplitStr(Reply, '\n')) {
        if (not ParseSatipServer(line))
           continue;
        Message("using " + WirbelscanSetup.SatipDesc + "@" + WirbelscanSetup.SatipAddr);
        }

     if (WirbelscanSetup.SatipAddr.empty()) {
        Message("no satip server found.");
        return -1;
        }
     }

  for(int i = 0; i < cDevice::NumDevices(); i++) {
     cDevice* d = cDevice::GetDevice(i);
     cDvbDevice* dvb = dynamic_cast<cDvbDevice*>(d);

     std::string name = "Device" +  std::to_string(i);
     if (d->NumProvidedSystems() < 1) {
        name += ": Device or resource busy";
        }
     else if (*d->DeviceName() != nullptr) {
        if (dvb) {
           name += " dvb:";
           name += "a" + std::to_string(dvb->Adapter());
           name += "f" + std::to_string(dvb->Frontend());
           name += ": ";
           }
        else
           name += " plg: ";
        name += *d->DeviceName();
        }
     Message(name);
     }

  if (OutputFormat == "FEMON")
     return SignalMonitor(cDevice::GetDevice(0), WirbelscanSetup.FemonChannel);

  Message("/*******************************************************************************");
  Message(" * Scan Type");
  Message(" ******************************************************************************/");
  switch(WirbelscanSetup.DVB_Type) {
     case 5 /* A */: {
        Message("  ATSC");
        Message("  country = " + CountryNames[WirbelscanSetup.CountryIndex]);
        break;
        }
     case 1 /* C */: {
        Message("  DVB-C");
        Message("  country = " + CountryNames[WirbelscanSetup.CountryIndex]);
        break;
        }
     case 2 /* S */: {
        Message("  DVB-S/S2");
        Message("  satellite = " + SatNames[WirbelscanSetup.SatIndex]);
        PrintLnb();
        if (Setup.DiSEqC)
           PrintDiseqc();
        break;
        }
     default/* T */: {
        Message("  DVB-T/T2");
        Message("  country = " + CountryNames[WirbelscanSetup.CountryIndex]);
        break;
        }
     }

  auto Start = std::chrono::high_resolution_clock::now();
  bool Continue = StartScan();

  while(Continue) {
     mSleep(500);
     Continue = Scanning();
     }

  std::vector<TChannel> data;
  if (Import(data) and not data.empty()) {
     auto Stop = std::chrono::high_resolution_clock::now();
     auto Elapsed = std::chrono::duration_cast<std::chrono::seconds>(Stop - Start).count();

     if (WirbelscanSetup.ParseLCN and WirbelscanSetup.verbosity > 4) {
        extern std::string VdrChannel(TChannel c);
        extern std::vector<TChannel> UniqueChannels(const std::vector<TChannel>& List);
        for(auto c:UniqueChannels(data)) dlog(5, BackFill(IntToStr(c.LCN),6) + VdrChannel(c));
        }

     Message(std::to_string(data.size()) + " services. scan time: " +
             IntToStr(Elapsed / 60, 2, false, '0') + ":" + IntToStr(Elapsed % 60, 2, false, '0'));

     if      (OutputFormat == "VDR")            PrintVDR(data);
     else if (OutputFormat == "INI")            PrintIni(data);
     else if (OutputFormat == "VLC")            PrintVLC(data);
     else if (OutputFormat == "VLC_SAT>IP")     PrintVLC(data, true);
     else if (OutputFormat == "XINE")           PrintXine(data);
     else if (OutputFormat == "MPLAYER")        PrintXine(data, true);
     else if (OutputFormat == "INITIAL")        PrintInitial(data);
     else if (OutputFormat == "XML")            PrintXML(data);
     else if (OutputFormat == "DAT")            PrintDat(data, WirbelscanSetup.OutputFile);
     else if (OutputFormat == "SATELLITES.DAT") PrintSatellitesDat(data);
     else Message("no output format defined - try again!");
     }

  for(auto l:libs)
     l->Plugin()->Stop();

  UnloadLibraries();
}
