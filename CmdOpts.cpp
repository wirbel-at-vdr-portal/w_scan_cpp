/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#include <string>
#include <vector>
#include <sstream>

#include "CmdOpts.h"
#include "Helpers.h"
#include "Satellites.h"
#include "Countries.h"
#include "Library.h"
#include "Lnb.h"
#include "DiSEqC.h"


bool HelpText(std::string ProgName);
bool ExtHelpText(std::string ProgName);

cPlugin* wirbelscan;
cPlugin* satip;

std::string OutputFormat("VDR");
std::string PlugPath("./vdr/PLUGINS/lib/");
std::string LibWirbelscan(PlugPath + "libvdr-wirbelscan_cmd.so." + std::string(APIVERSION));
std::string LibSatip     (PlugPath + "libvdr-satip_cmd.so."      + std::string(APIVERSION));



#define PARAM(s)                                                 \
  if (not CheckParam(Param, s, i)) {                             \
     if (Param.empty())                                          \
        ErrorMessage("missing parameter");                       \
     else                                                        \
        ErrorMessage("invalid parameter '" + Param + "'");       \
     return false;                                               \
     }

bool CheckParam(std::string& Param, std::string Values, size_t& i) {
  bool valid = false;
  for(auto s:SplitStr(Values,',')) if (Param == s) { valid = true; i++; break; }
  return valid;
}

std::string IntRange(int from, int to) {
  std::string result;
  for(int i = from; i <= to; i++)
     if (result.empty()) result  =       std::to_string(i);
     else                result += "," + std::to_string(i);
  return result;
}

wsetup WirbelscanSetup;

void WirbelscanDefaults(void) {
  WirbelscanSetup.ATSC_type = 0;
  WirbelscanSetup.CountryIndex = GetCountryId("DE");
  WirbelscanSetup.DVB_Type = 0;
  WirbelscanSetup.DVBC_Inversion = 0;
  WirbelscanSetup.DVBC_Network_PID = 0x10;
  WirbelscanSetup.DVBC_QAM = 0;
  WirbelscanSetup.DVBC_Symbolrate = 0;
  WirbelscanSetup.DVBT_Inversion = 0;
  WirbelscanSetup.enable_s2 = 1;
  WirbelscanSetup.logFile = 3;
  WirbelscanSetup.verbosity = 4;
  WirbelscanSetup.scan_remove_invalid = 0;
  WirbelscanSetup.SatIndex = GetSatelliteId("S19E2");
  WirbelscanSetup.scanflags = 15;
//WirbelscanSetup.SingleTransponder = { "T:682000:0:B8I1M2S0","C:306:6900:I1M256","S19E2:10714:22000:HC23M5O35P0S1","P:0:0:","P:0:0:","A:306:0:M2" };
//WirbelscanSetup.tp_only = 0;
  WirbelscanSetup.scan_update_existing = 0;
  WirbelscanSetup.scan_append_new = 1;
  WirbelscanSetup.HelpText = false;
  WirbelscanSetup.ParseLCN = false;
}

void WirbelscanUpdateSettings(void) {
  #define SendInteger(b,c) wirbelscan->SetupParse(b,std::to_string(c).c_str())
  SendInteger("ATSC_type"         , WirbelscanSetup.ATSC_type);
  SendInteger("CountryIndex"      , WirbelscanSetup.CountryIndex);
  SendInteger("DVB_Type"          , WirbelscanSetup.DVB_Type);
  SendInteger("DVBC_Inversion"    , WirbelscanSetup.DVBC_Inversion);
  SendInteger("DVBC_Network_PID"  , WirbelscanSetup.DVBC_Network_PID);
  SendInteger("DVBC_QAM"          , WirbelscanSetup.DVBC_QAM);
  SendInteger("DVBC_Symbolrate"   , WirbelscanSetup.DVBC_Symbolrate);
  SendInteger("DVBT_Inversion"    , WirbelscanSetup.DVBT_Inversion);
  SendInteger("logFile"           , WirbelscanSetup.logFile);
  SendInteger("SatIndex"          , WirbelscanSetup.SatIndex);
  SendInteger("scanflags"         , WirbelscanSetup.scanflags);
//            "SingleTransponder" , "T:682000:0:B8I1M2S0|C:306:6900:I1M256|S19E2:10714:22000:HC23M5O35P0S1|P:0:0:|P:0:0:|A:306:0:M2"
//SendInteger("tp_only"           , WirbelscanSetup.tp_only);
  SendInteger("verbosity"         , WirbelscanSetup.verbosity);
  SendInteger("ParseLCN"          , WirbelscanSetup.ParseLCN?1:0);
  #undef SendInteger
}

void VdrDefaults(void) {
  Setup.DeviceBondings = "";
  Setup.LnbSLOF    = 11700;
  Setup.LnbFrequLo =  9750;
  Setup.LnbFrequHi = 10600;
  Setup.DiSEqC = 0;
  Setup.UsePositioner = 0;
  Setup.SiteLat = 0;
  Setup.SiteLon = 0;
  Setup.PositionerLastLon = 0;
  Setup.PositionerSpeed = 15;
  Setup.PositionerSwing = 650;
  Setup.PrimaryDVB = 1;
  Setup.StandardCompliance = STANDARD_DVB;//0
  Setup.UpdateChannels = 0;
}



bool ParseArguments(int argc, char* argv[]) {
  std::vector<std::string> Arguments;
  std::string ProgName(argv[0]);
  std::string DiseqcSwitch;
  std::string RotorUsals;
  std::string Scr;
  std::string Source("S19.2E");
  int RotorPosition = 9999;
  bool use_satip = false;

  size_t p = ProgName.find_last_of("/\\");
  if (p != std::string::npos)
     ProgName.erase(0, p);

  libs.push_back(new Library(LibWirbelscan, ""));

  wirbelscan = libs[0]->Plugin();
  satip      = nullptr;

  if (wirbelscan) {
     InitCountries(wirbelscan);
     InitSatellites(wirbelscan);
     }

  WirbelscanDefaults();
  Diseqcs.Load(nullptr);

  // accept short args w/o space in between
  for(int i=1; i<argc; i++) {
     std::string s = argv[i];
     if (s.size() > 2 and s[0] == '-' and s[1] != '-') {
        Arguments.push_back(s.substr(0, 2));
        Arguments.push_back(s.substr(2));
        }
     else
        Arguments.push_back(s);
     }

  for(size_t i=0; i<Arguments.size(); i++) {
     std::string Argument = Arguments[i];
     std::string Param;

     if ((i+1) < Arguments.size())
        Param = Arguments[i+1];

     if ((Argument == "-h") or (Argument == "--help"))
        return HelpText(ProgName);
     else if ((Argument == "-H") or (Argument == "--extended-help"))
        return ExtHelpText(ProgName);
     else if ((Argument == "-t") or (Argument == "--satip"))
        use_satip = true;
     else if ((Argument == "-f") or (Argument == "--frontend")) {
        PARAM("a,c,s,t");
        if      (Param == "a") WirbelscanSetup.DVB_Type = 5;
        else if (Param == "c") WirbelscanSetup.DVB_Type = 1;
        else if (Param == "s") WirbelscanSetup.DVB_Type = 2;
        else if (Param == "t") WirbelscanSetup.DVB_Type = 0;
        }
     else if ((Argument == "-A") or (Argument == "--atsc_type")) {
        PARAM("1,2,3");
        if      (Param == "1") WirbelscanSetup.ATSC_type = 0;
        else if (Param == "2") WirbelscanSetup.ATSC_type = 1;
        else if (Param == "3") WirbelscanSetup.ATSC_type = 2;
        }
     else if ((Argument == "-c") or (Argument == "--country")) {
        Param = UpperCase(Param);
        PARAM("?," + CountryArgs());
        if      (Param == "?") return PrintCountries();
        else    WirbelscanSetup.CountryIndex = GetCountryId(Param);
        }
     else if ((Argument == "-s") or (Argument == "--satellite")) {
        Param = UpperCase(Param);
        PARAM("?," + SatArgs());
        if      (Param == "?") return PrintSatellites();
        else {
           WirbelscanSetup.SatIndex = GetSatelliteId(Param);
           Source = VdrSource(Param);
           }
        }
     // ----------------output switches-----------------
     else if ((Argument == "-I") or (Argument == "--output-ini")) {
        OutputFormat = "INI";
        }
     else if ((Argument == "-L") or (Argument == "--output-VLC")) {
        OutputFormat = "VLC";
        }
     else if ((Argument == "-V") or (Argument == "--output-VLC-satip")) {
        OutputFormat = "VLC_SAT>IP";
        }
     else if ((Argument == "-M") or (Argument == "--output-mplayer")) {
        OutputFormat = "MPLAYER";
        }
     else if (Argument == "--output-satellites.dat") {
        OutputFormat = "SATELLITES.DAT";
        }
     else if ((Argument == "-X") or (Argument == "--output-xine")) {
        OutputFormat = "XINE";
        }
     else if ((Argument == "-x") or (Argument == "--output-initial")) {
        OutputFormat = "INITIAL";
        }
     else if ((Argument == "-Z") or (Argument == "--output-xml")) {
        OutputFormat = "XML";
        }
     else if ((Argument == "-Y") or (Argument == "--output-dat")) {
        OutputFormat = "DAT";
        if (not Param.empty()) {
           WirbelscanSetup.OutputFile = Param;
           i++;
           }
        else {
           ErrorMessage("missing parameter FILENAME");
           return false;
           }
        }
     else if ((Argument == "-F") or (Argument == "--femon")) {
        OutputFormat = "FEMON";
        if (not Param.empty()) {
           WirbelscanSetup.FemonChannel = Param;
           i++;
           }
        else {
           ErrorMessage("missing parameter VDR-Channel");
           return false;
           }
        }
     else if ((Argument == "-v") or (Argument == "--verbose"))
        WirbelscanSetup.verbosity++;
     else if ((Argument == "-q") or (Argument == "--quiet"))
        WirbelscanSetup.verbosity--;

     else if ((Argument == "-C") or (Argument == "--sort-criteria")) {
        WirbelscanSetup.SortCriteria = ReadFile(Param), i++;
        }
     else if ((Argument == "-R") or (Argument == "--radio-services")) {
        PARAM("0,1");
        WirbelscanSetup.scanflags &= ~(1 << 1);
        WirbelscanSetup.scanflags |= std::stol(Param) << 1;
        }
     else if ((Argument == "-T") or (Argument == "--tv-services")) {
        PARAM("0,1");
        WirbelscanSetup.scanflags &= ~(1 << 0);
        WirbelscanSetup.scanflags |= std::stol(Param) << 0;
        }
     else if ((Argument == "-E") or (Argument == "--encrypted-services")) {
        PARAM("0,1");
        WirbelscanSetup.scanflags &= ~(1 << 3);
        WirbelscanSetup.scanflags |= std::stol(Param) << 3;
        }
     else if ((Argument == "-a") or (Argument == "--adapter")) {
        WirbelscanSetup.adapter = Param;
        i++;
        }
     else if (Argument == "--satip-server") {
        WirbelscanSetup.SatipSvr = Param;
        i++;
        }
     else if ((Argument == "-i") or (Argument == "--inversion")) {
        PARAM("0,1,2");
        if      (Param == "0") WirbelscanSetup.DVBC_Inversion = 0;
        else if (Param == "1") WirbelscanSetup.DVBC_Inversion = 1;
        else if (Param == "2") WirbelscanSetup.DVBC_Inversion = 999;
        }
     else if ((Argument == "-Q") or (Argument == "--dvbc-modulation")) {
        PARAM("0,1,2,3");
        if      (Param == "0") WirbelscanSetup.DVBC_QAM = 1; //64
        else if (Param == "1") WirbelscanSetup.DVBC_QAM = 3; //256
        else if (Param == "2") WirbelscanSetup.DVBC_QAM = 2; //128
        else if (Param == "3") WirbelscanSetup.DVBC_QAM = 4; //64+128+256
        }
     else if ((Argument == "-e") or (Argument == "--dvbc-extflags")) {
        PARAM("0,1,2,3");
        if      (Param == "0") { WirbelscanSetup.DVBC_QAM = 999; WirbelscanSetup.DVBC_Symbolrate =  0; }
        else if (Param == "1") { WirbelscanSetup.DVBC_QAM = 999; WirbelscanSetup.DVBC_Symbolrate = 16; }
        else if (Param == "2") { WirbelscanSetup.DVBC_QAM = 4;   WirbelscanSetup.DVBC_Symbolrate =  0; }
        else if (Param == "3") { WirbelscanSetup.DVBC_QAM = 4;   WirbelscanSetup.DVBC_Symbolrate = 16; }
        }
     else if ((Argument == "-S") or (Argument == "--dvbc-symbolrate")) {
        PARAM("0,1,4,5,6,7,8,9,11,12,14,15,16,17");
        if      (Param == "0") WirbelscanSetup.DVBC_Symbolrate =  1; //6900
        else if (Param == "1") WirbelscanSetup.DVBC_Symbolrate =  2; //6875
      //else if (Param == "2") WirbelscanSetup.DVBC_Symbolrate =     //6956.5
      //else if (Param == "3") WirbelscanSetup.DVBC_Symbolrate =     //6956
        else if (Param == "4") WirbelscanSetup.DVBC_Symbolrate = 13; //6952
        else if (Param == "5") WirbelscanSetup.DVBC_Symbolrate = 11; //6950
        else if (Param == "6") WirbelscanSetup.DVBC_Symbolrate =  5; //6790
        else if (Param == "7") WirbelscanSetup.DVBC_Symbolrate =  6; //6811
        else if (Param == "8") WirbelscanSetup.DVBC_Symbolrate =  4; //6250
        else if (Param == "9") WirbelscanSetup.DVBC_Symbolrate =  3; //6111
      //else if (Param == "10")WirbelscanSetup.DVBC_Symbolrate =     //6086
        else if (Param == "11")WirbelscanSetup.DVBC_Symbolrate =  7; //5900
        else if (Param == "12")WirbelscanSetup.DVBC_Symbolrate = 15; //5483
      //else if (Param == "13")WirbelscanSetup.DVBC_Symbolrate =     //5217
        else if (Param == "14")WirbelscanSetup.DVBC_Symbolrate = 14; //5156
        else if (Param == "15")WirbelscanSetup.DVBC_Symbolrate =  8; //5000
        else if (Param == "16")WirbelscanSetup.DVBC_Symbolrate = 10; //4000
        else if (Param == "17")WirbelscanSetup.DVBC_Symbolrate =  9; //3450
        }
     else if ((Argument == "-l") or (Argument == "--lnb-type")) {
        if ((not Param.empty()) and (Param.find_first_not_of("0123456789,") == std::string::npos)) {
           if (not SetLnb(Param)) return true;
           i++;
           }
        else {
           PARAM("?,UNIVERSAL,DBS,STANDARD,ENHANCED,C-BAND,C-MULTI,AUSTRALIA");
           if (not SetLnb(Param)) return true;
           }
        }
     else if ((Argument == "-D") or (Argument == "--diseqc-switch")) {
        PARAM("0c,1c,2c,3c,0u,1u,2u,3u,4u,5u,6u,7u,8u,9u,10u,11u,12u,13u,14u,15u");
        DiseqcSwitch = Param;        
        }
     else if ((Argument == "-r") or (Argument == "--rotor-position")) {
        PARAM(IntRange(0,360));
        RotorPosition = std::stol(Param);
        }
     else if ((Argument == "-U") or (Argument == "--rotor-usals")) {
        auto l = SplitStr(Param,':');
        if (l.size() != 4) {
           ErrorMessage("invalid parameter '" + Param + "' - wrong number of items.");
           return false;
           }
        for(auto s:l)
           if (s.empty() or (s.find_first_not_of("0123456789") != std::string::npos)) {
              ErrorMessage("invalid parameter '" + s + "': -> not integer");
              return false;
              }
        RotorUsals = Param;
        i++;
        }
     else if ((Argument == "-u") or (Argument == "--scr")) {
        std::string s(Param);
        auto l = SplitStr(s,':');
        if (l.size() == 3) {
           Param = l[0]; PARAM(IntRange(0, 31));     --i;
           Param = l[1]; PARAM(IntRange(950, 2150)); --i;
           Param = l[2]; PARAM("A,B,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p"); --i;
           Param = s;
           }
        else if (l.size() == 4) {
           Param = l[0]; PARAM(IntRange(0, 31));     --i;
           Param = l[1]; PARAM(IntRange(950, 2150)); --i;
           Param = l[2]; PARAM("A,B,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p"); --i;
           Param = l[3]; PARAM(IntRange(0, 255));    --i;
           Param = s;
           }
        else {
           ErrorMessage("invalid parameter '" + Param + "' - wrong number of items.");
           return false;
           }
        Scr = Param;
        i++;
        }
     else if (Argument == "--enable-lcn")
        WirbelscanSetup.ParseLCN = true;
     else {
        ErrorMessage("cannot understand '" + Argument + "'");
        return false;
        }
     }

  if (use_satip) {
     std::string options("-t 16384");

     // add further satip plugin options as needed.
     if (not WirbelscanSetup.SatipSvr.empty())
        options += ";-s" + WirbelscanSetup.SatipSvr;

     libs.push_back(new Library(LibSatip, options));
     satip = libs[1]->Plugin();
     }

  switch(WirbelscanSetup.DVB_Type) {
     case 5 /* A */: break;
     case 1 /* C */: break;
     case 2 /* S */:
        {
        if (not DiseqcSwitch.empty())
           DiseqcSwitchConfig(Source, Setup.LnbFrequLo, Setup.LnbFrequHi, Setup.LnbSLOF, DiseqcSwitch);
        else if (not Scr.empty())
           DiseqcScr(Source, Setup.LnbFrequLo, Setup.LnbFrequHi, Setup.LnbSLOF, Scr);
        else if (RotorPosition != 9999)
           DiseqcRotorPosition(Source, Setup.LnbFrequLo, Setup.LnbFrequHi, Setup.LnbSLOF, RotorPosition);
        else if (not RotorUsals.empty())
           DiseqcRotorUsals(Setup.LnbFrequLo, Setup.LnbFrequHi, Setup.LnbSLOF, RotorUsals);

        break;
        }
     default/* T */: break;
     }


  WirbelscanUpdateSettings();
  return true;
}




bool HelpText(std::string ProgName) {
  std::stringstream ss;
  ss << "usage: " << ProgName << " [options...] " << std::endl;
  ss << "       -f type, --frontend type" << std::endl;
  ss << "               What programs do you want to search for?" << std::endl;
  ss << "               a = atsc (vsb/qam)" << std::endl;
  ss << "               c = cable " << std::endl;
  ss << "               s = sat " << std::endl;
  ss << "               t = terrestrial [default]" << std::endl;
  ss << "       -A N, --atsc_type N" << std::endl;
  ss << "               specify ATSC type" << std::endl;
  ss << "               1 = Terrestrial [default]" << std::endl;
  ss << "               2 = Cable" << std::endl;
  ss << "               3 = both, Terrestrial and Cable" << std::endl;
  ss << "       -t, --satip" << std::endl;
  ss << "               use SAT>IP tuner via VDR Satip Plugin" << std::endl;
  ss << "               see https://www.satip.info" << std::endl;
  ss << "       -c, --country" << std::endl;
  ss << "               choose your country here:" << std::endl;
  ss << "                       DE, GB, US, AU, .." << std::endl;
  ss << "                       ? for list" << std::endl;
  ss << "               " << std::endl;
  ss << "       -s, --satellite" << std::endl;
  ss << "               choose your satellite here:" << std::endl;
  ss << "                       S19E2, S13E0, S15W0, .." << std::endl;
  ss << "                       ? for list" << std::endl;
  ss << "               ---output switches---" << std::endl;
  ss << "       -I, --output-ini" << std::endl;
  ss << "               generate transponder ini" << std::endl;
  ss << "       -L, --output-VLC" << std::endl;
  ss << "               generate VLC xspf playlist" << std::endl;
  ss << "       -V, --output-VLC-satip" << std::endl;
  ss << "               generate VLC xspf playlist for SAT>IP" << std::endl;
  ss << "       -M, --output-mplayer" << std::endl;
  ss << "               mplayer output instead of vdr channels.conf" << std::endl;
  ss << "       -X, --output-xine" << std::endl;
  ss << "               tzap/czap/xine output instead of vdr channels.conf" << std::endl;
  ss << "       -x, --output-initial" << std::endl;
  ss << "               generate dtv scan table for dvbv5-scan" << std::endl;
  ss << "       -Z, --output-xml" << std::endl;
  ss << "               generate w_scan XML tuning data" << std::endl;
  ss << "       -Y <FILENAME>, --output-dat <FILENAME>" << std::endl;
  ss << "               generate channels.dat for SAT>IP DvbViewer Lite" << std::endl;
  ss << "       -F <VDR_Channel>, --femon <VDR_Channel>" << std::endl;
  ss << "               monitor signal lock, strength and quality" << std::endl;
  ss << "               of a given channel in VDR format" << std::endl;
  ss << "       -H, --extended-help" << std::endl;
  ss << "               view extended help (experts only)" << std::endl;
  std::cout << ss.str();
  WirbelscanSetup.HelpText = true;
  return true;
}

bool ExtHelpText(std::string ProgName) {
  std::stringstream ss;
  ss << "*** expert help ***" << std::endl;
  ss << ".................General................." << std::endl;
  ss << "       -v, --verbose" << std::endl;
  ss << "               be more verbose (repeat for more)" << std::endl;
  ss << "       -q, --quiet" << std::endl;
  ss << "               be more quiet   (repeat for less)" << std::endl;
  ss << ".................Services................" << std::endl;
  ss << "       -C, --sort-criteria FILE" << std::endl;
  ss << "               sort output as given by FILE." << std::endl;
  ss << "               For details, see w_scan_cpp man page." << std::endl;
  ss << "       -R N, --radio-services N" << std::endl;
  ss << "               0 = don't search radio channels" << std::endl;
  ss << "               1 = search radio channels [default]" << std::endl;
  ss << "       -T N, --tv-services N" << std::endl;
  ss << "               0 = don't search TV channels" << std::endl;
  ss << "               1 = search TV channels[default]" << std::endl;
  ss << "       -E N, --encrypted-services (Conditional Access)" << std::endl;
  ss << "               N=0 gets only Free TV channels" << std::endl;
  ss << "               N=1 search also encrypted channels [default]" << std::endl;
  ss << "       --enable-lcn" << std::endl;
  ss << "               enable 'logical channel numbering'" << std::endl;
  ss << "               Logical channel numbering (LCN) is used in very rare" << std::endl;
  ss << "               countries and a non-standard feature of DTV." << std::endl;
  ss << "               If you are using an terrestrial or cable based setup," << std::endl;
  ss << "               and live in one of the countries which is using LCN," << std::endl;
  ss << "               you may enable this feature here." << std::endl;
  ss << "               LCN is off by default." << std::endl;
  ss << ".................Device.................." << std::endl;
  ss << "       -a N, --adapter N" << std::endl;
  ss << "               use device /dev/dvb/adapterN/ [default: auto detect]" << std::endl;
  ss << "               (also allowed: -a /dev/dvb/adapterN/frontendM)" << std::endl;
  ss << "       --satip-server <STRING>" << std::endl;
  ss << "               do not auto detect satip server," << std::endl;
  ss << "               but use manual server settings, ie." << std::endl;
  ss << "                 192.168.2.66|DVBC-1" << std::endl;
  ss << "                 192.168.2.66|DVBS2-2,DVBT2-2|OctopusNet" << std::endl;
  ss << "                 192.168.2.66|DVBS2-4|OctopusNet;192.168.0.2|DVBT2-4|minisatip:0x18" << std::endl;
  ss << "                 192.168.2.66:554|DVBS2-2:S19.2E|OctopusNet;192.168.2.2:8554|DVBS2-4:S19.2E,S1W|minisatip" << std::endl;
  ss << "               for detailed description, refer to vdr-plugin-satip's README." << std::endl;
  ss << ".................DVB-C..................." << std::endl;
  ss << "       -i N, --inversion N" << std::endl;
  ss << "               spectral inversion setting for cable TV" << std::endl;
  ss << "                       (0: off, 1: on, 2: auto [default])" << std::endl;
  ss << "       -Q N, --dvbc-modulation N" << std::endl;
  ss << "               set DVB-C modulation, see table:" << std::endl;
  ss << "                       0  = QAM64" << std::endl;
  ss << "                       1  = QAM256" << std::endl;
  ss << "                       2  = QAM128" << std::endl;
  ss << "                       3  = all (SLOW!)" << std::endl;
  ss << "               NOTE: for experienced users only!!" << std::endl;
  ss << "       -e N,--dvbc-extflags N" << std::endl;
  ss << "               extended scan flags (DVB-C only)," << std::endl;
  ss << "               Any combination of these flags:" << std::endl;
  ss << "               1 = use extended symbolrate list" << std::endl;
  ss << "                       enables scan of symbolrates" << std::endl;
  ss << "                       6900, 6875, 6111, 6250, 6790," << std::endl;
  ss << "                       6811, 5900, 5000, 3450, 4000," << std::endl;
  ss << "                       6950, 7000, 6952, 5156, 5483" << std::endl;
  ss << "               2 = extended QAM scan (enable QAM128)" << std::endl;
  ss << "                       recommended for Nethterlands and Finland" << std::endl;
  ss << "               NOTE: extended scan will be *slow*" << std::endl;
  ss << "       -S N, dvbc-symbolrate N" << std::endl;
  ss << "               set DVB-C symbol rate, see table:" << std::endl;
  ss << "                       0  = 6900 kSymbol/s" << std::endl;
  ss << "                       1  = 6875 kSymbol/s" << std::endl;
  ss << "                       4  = 6952 kSymbol/s" << std::endl;
  ss << "                       5  = 6950 kSymbol/s" << std::endl;
  ss << "                       6  = 6790 kSymbol/s" << std::endl;
  ss << "                       7  = 6811 kSymbol/s" << std::endl;
  ss << "                       8  = 6250 kSymbol/s" << std::endl;
  ss << "                       9  = 6111 kSymbol/s" << std::endl;
  ss << "                       11 = 5900 kSymbol/s" << std::endl;
  ss << "                       12 = 5483 kSymbol/s" << std::endl;
  ss << "                       14 = 5156 kSymbol/s" << std::endl;
  ss << "                       15 = 5000 kSymbol/s" << std::endl;
  ss << "                       16 = 4000 kSymbol/s" << std::endl;
  ss << "                       17 = 3450 kSymbol/s" << std::endl;
  ss << "               NOTE: for experienced users only!!" << std::endl;
  ss << ".................DVB-S/S2................" << std::endl;
  ss << "       -l <LNB type>, --lnb-type <LNB type>" << std::endl;
  ss << "               choose LNB type by name (DVB-S/S2 only)" << std::endl;
  ss << "                       ? for list" << std::endl;
  ss << "       -D Nc, --diseqc-switch Nc" << std::endl;
  ss << "               use DiSEqC committed switch position N" << std::endl;
  ss << "               AA..BB => 0..3" << std::endl;
  ss << "       -D Nu, --diseqc-switch Nu" << std::endl;
  ss << "               use DiSEqC uncommitted switch position N" << std::endl;
  ss << "               N = 0..15" << std::endl;
  ss << "       -r N, --rotor-position N" << std::endl;
  ss << "               use Rotor position N (needs -s)" << std::endl;
  ss << "       -U, --rotor-usals PARAMLIST" << std::endl;
  ss << "               where PARAMLIST is Lat:Long:Speed:Swing" << std::endl;
  ss << "                 Lat  : your site latitude  in tenth of degree, negative south, positive north" << std::endl;
  ss << "                 Long : your site longitude in tenth of degree, negative west , positive east"  << std::endl;
  ss << "                 Speed: your rotor speed in tenth of degree per second" << std::endl;
  ss << "                 Swing: your rotor max swing in tenth of degree" << std::endl;
  ss << "                 example: a rotor located in Berlin/Germany (52.52,13.41)," << std::endl;
  ss << "                          moving at 1.5deg/sec, max swing 65deg gives" << std::endl;
  ss << "                          --rotor-usals 525:134:15:650" << std::endl;
  ss << "       -u    <slot:user_frequency:sat_pos(:user_pin)>" << std::endl;
  ss << "       --scr <slot:user_frequency:sat_pos(:user_pin)>" << std::endl;
  ss << "               Satellite Channel Routing" << std::endl;
  ss << "                      a) use EN50494:" << std::endl;
  ss << "                         slot          :  slot number for user frequency, 0..7" << std::endl;
  ss << "                         user_frequency:  receiver user frequency for slot in MHz, i.e. 1400" << std::endl;
  ss << "                         sat_pos       :  satellite position (upper case), 'A' or 'B'" << std::endl;
  ss << "                         user_pin      :  optional user pin, normally not used (0..255)" << std::endl;
  ss << "                      i.e. -u 0:1400:A for EN50494, slot 0 at 1400 MHz, Satellite Pos 'A'" << std::endl;
  ss << "                      " << std::endl;
  ss << "                      b) use advanced SCR EN50607/JESS:" << std::endl;
  ss << "                         slot          :  slot number for user frequency, 0..31" << std::endl;
  ss << "                         user_frequency:  receiver user frequency for slot in MHz, i.e. 1400" << std::endl;
  ss << "                         sat_pos       :  satellite position (lower case), 'a' .. 'p'" << std::endl;
  ss << "                         user_pin      :  optional user pin, normally not used (0..255)" << std::endl;
  ss << "                      i.e. -u 0:1400:a for EN50607 slot 0 at 1400 MHz, Satellite Pos 'a'" << std::endl;
  ss << "                                             sat| committed switch  | uncommitted switch" << std::endl;
  ss << "                                             pos| option | position | option | position" << std::endl;
  ss << "                                              'a'    0   |   0      |   0    |   0" << std::endl;
  ss << "                                              'b'    0   |   1      |   0    |   0" << std::endl;
  ss << "                                              'c'    1   |   0      |   0    |   0" << std::endl;
  ss << "                                              'd'    1   |   1      |   0    |   0" << std::endl;
  ss << "                                              'e'    0   |   0      |   0    |   1" << std::endl;
  ss << "                                              'f'    0   |   1      |   0    |   1" << std::endl;
  ss << "                                              'g'    1   |   0      |   0    |   1" << std::endl;
  ss << "                                              'h'    1   |   1      |   0    |   1" << std::endl;
  ss << "                                              'i'    0   |   0      |   1    |   0" << std::endl;
  ss << "                                              'j'    0   |   1      |   1    |   0" << std::endl;
  ss << "                                              'k'    1   |   0      |   1    |   0" << std::endl;
  ss << "                                              'l'    1   |   1      |   1    |   0" << std::endl;
  ss << "                                              'm'    0   |   0      |   1    |   1" << std::endl;
  ss << "                                              'n'    0   |   1      |   1    |   1" << std::endl;
  ss << "                                              'o'    1   |   0      |   1    |   1" << std::endl;
  ss << "                                              'p'    1   |   1      |   1    |   1" << std::endl;

  HelpText(ProgName);
  std::cout << ss.str();
  return true;
}

bool ParseSatipServer(std::string s) {
  auto items = SplitStr(s, '|');

  if ((s.find('+') != 0) or (items.size() != 3))
     return false;

  size_t pos = items[0].find_last_of(" @");
  WirbelscanSetup.SatipAddr  = items[0].substr(pos + 1);
  WirbelscanSetup.SatipModel = items[1];
  WirbelscanSetup.SatipDesc  = items[2];

  return true;
}
