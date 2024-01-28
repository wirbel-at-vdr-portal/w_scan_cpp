/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cctype>
#include "Helpers.h"
#include "OutputFormats.h"
#include "DiSEqC.h"
#include "Lnb.h"
#include "CmdOpts.h"

extern std::vector<std::string> SatNames;

std::string Param(TChannel c);
std::vector<TChannel> UniqueTransponders(const std::vector<TChannel>& List);
std::vector<TChannel> UniqueChannels(const std::vector<TChannel>& List);
std::string VdrChannel(TChannel c);
std::string CaName(int id);
#define INDENT std::string((size_t) (indent * 3), ' ')

struct {
  bool operator()(TChannel a, TChannel b) const {
     if (a.Source     != b.Source)      return a.Source     < b.Source;
     if (a.Frequency  != b.Frequency)   return a.Frequency  < b.Frequency;
     if (a.Symbolrate != b.Symbolrate)  return a.Symbolrate < b.Symbolrate;
     return (Param(a) < Param(b)) or (a.Name < b.Name);
     }
} CompareTransponder;

struct {
  bool operator()(TChannel a, TChannel b) const {
     if (WirbelscanSetup.SortCriteria.size()) {
        int index = 0;
        int A = ~0;
        int B = ~0;
        for(auto s:WirbelscanSetup.SortCriteria) {
           if      (UpperCase(s) == UpperCase(a.Name)) A = index;
           else if (UpperCase(s) == UpperCase(b.Name)) B = index;
           index++;
           }
        if ((A != ~0) and (B == ~0))
           return true;
        else if ((A == ~0) and (B != ~0))
           return false;
        else if ((A != ~0) and (B != ~0))
           return A < B;
        }
     if ((a.LCN != -1) or (b.LCN != -1)) {
        if (a.LCN == -1) return false; // b has valid LCN
        if (b.LCN == -1) return true;  // a has valid LCN
        // sort LCN ascending
        return (a.LCN < b.LCN) or ((a.LCN == b.LCN) and
               (a.LCN_minor < b.LCN_minor));
        }
     return UpperCase(VdrChannel(a)) < UpperCase(VdrChannel(b));
     }   
} CompareChannel;

struct {
  bool operator()(TChannel a, TChannel b) const {
     if (a.Source     != b.Source)      return false;
     if (a.Frequency  != b.Frequency)   return false;
     if (a.Symbolrate != b.Symbolrate)  return false;
     return Param(a) == Param(b);
     }   
} EqualTransponder;

struct {
  bool operator()(TChannel a, TChannel b) const {
     return a.LCN == b.LCN and a.LCN_minor == b.LCN_minor and
            UpperCase(VdrChannel(a)) == UpperCase(VdrChannel(b));
     }
} EqualChannel;

std::vector<TChannel> UniqueTransponders(const std::vector<TChannel>& List) {
  std::vector<TChannel> Result = List;
  std::sort(Result.begin(), Result.end(), CompareTransponder);
  Result.erase(std::unique(Result.begin(), Result.end(), EqualTransponder), Result.end());
  return Result;
}

std::vector<TChannel> UniqueChannels(const std::vector<TChannel>& List) {
  std::vector<TChannel> Result = List;
  std::sort(Result.begin(), Result.end(), CompareChannel);
  Result.erase(std::unique(Result.begin(), Result.end(), EqualChannel), Result.end());
  return Result;
}

std::string Param(TChannel c) {
  std::stringstream ss;
  if (c.Source == "A") {
     if (c.Inversion    != 999) ss << "I" << c.Inversion;
     if (c.Modulation   != 999) ss << "M" << c.Modulation;
     }
  else if (c.Source == "C") {
     if (c.Inversion    != 999) ss << "I" << c.Inversion;
     if (c.FEC          != 999) ss << "C" << c.FEC;
     if (c.Modulation   != 999) ss << "M" << c.Modulation;
     }
  else if (c.Source == "T") {
     if (c.Inversion    != 999) ss << "I" << c.Inversion;
     if (c.Bandwidth    != 999) ss << "B" << c.Bandwidth;
     if (c.FEC          != 999) ss << "C" << c.FEC;
     if (c.FEC_low      != 999) ss << "D" << c.FEC_low;
     if (c.Guard        != 999) ss << "G" << c.Guard;
     if (c.Modulation   != 999) ss << "M" << c.Modulation;
     if (c.DelSys       != 999) ss << "S" << c.DelSys;
     if (c.Transmission != 999) ss << "T" << c.Transmission;
     if (c.Hierarchy    != 999) ss << "Y" << c.Hierarchy;
     if (c.DelSys == 1) {
        if (c.StreamId  != 999) ss << "P" << c.StreamId;
        if (c.SystemId  != 999) ss << "Q" << c.SystemId;
        if (c.MISO      != 999) ss << "X" << c.MISO;
        }
     }
  else if (c.Source[0] == 'S') {
     ss << c.Polarization;
     if (c.Inversion    != 999) ss << "I" << c.Inversion;
     if (c.FEC          != 999) ss << "C" << c.FEC;
     if (c.Modulation   != 999) ss << "M" << c.Modulation;
     if (c.DelSys       != 999) ss << "S" << c.DelSys;
     if (c.DelSys == 1) {
        if (c.Pilot     != 999) ss << "N" << c.Pilot;
        if (c.Rolloff   != 999) ss << "O" << c.Rolloff;
        if (c.StreamId  != 999) ss << "P" << c.StreamId;
        }
     }
  return ss.str();
}

std::string VdrChannel(TChannel c) {
  std::stringstream ss;
  if (c.Name.empty()) c.Name = "???";

  /* The line "<NAME>,<SHORTNAME>;<PROVIDER>:<FREQUENCY>:(..)" is scanned from
   * left until the (leftmost) <FREQUENCY> ':' delimiter.
   * No ':' delimiters allowed here:
   */
  ReplaceAll(c.Name     , ":", "|");
  ReplaceAll(c.Shortname, ":", "|");
  ReplaceAll(c.Provider , ":", "|");

  /* The field  "<NAME>,<SHORTNAME>;<PROVIDER>" is scanned from left until
   * the (leftmost) <PROVIDER> ';' delimiter.  No ';' delimiters allowed here:
   */
  ReplaceAll(c.Name     , ";", "|");
  ReplaceAll(c.Shortname, ";", "|");

  /* The field  "<NAME>,<SHORTNAME>" is scanned from right until
   * the (rightmost) <SHORTNAME> ',' delimiter.
   * No ',' delimiter allowed here:
   */
  ReplaceAll(c.Shortname, ",", ".");

  ss << c.Name;
  if (c.Shortname.size())   ss << "," << c.Shortname;
  if (c.Provider.size())    ss << ";" << c.Provider;
  ss << ":" << c.Frequency << ":" << Param(c);
  ss << ":" << c.Source << ":" << c.Symbolrate << ":";
  ss << c.VPID.PID;
  if (c.PCR and c.PCR != c.VPID.PID) ss << "+" << c.PCR;
  if (c.VPID.Type)                   ss << "=" << c.VPID.Type;
  ss << ":";

  if (c.APIDs.Count()) {
     for(int i = 0; i < c.APIDs.Count(); i++) {
        if (i) ss << ",";
        ss << c.APIDs[i].PID;
        if (c.APIDs[i].Lang.size()) ss << "=" << c.APIDs[i].Lang;
        if (c.APIDs[i].Type)        ss << "@" << c.APIDs[i].Type;
        }
     }
  else ss << "0";

  if (c.DPIDs.Count()) {
     ss << ";";
     for(int i = 0; i < c.DPIDs.Count(); i++) {
        if (i) ss << ",";
        ss << c.DPIDs[i].PID;
        if (c.DPIDs[i].Lang.size()) ss << "=" << c.DPIDs[i].Lang;
        if (c.DPIDs[i].Type)        ss << "@" << c.DPIDs[i].Type;
        }
     }
  ss << ":" << c.TPID << ":" << std::hex;

  if (c.CAIDs.Count()) {
     for(int i = 0; i < c.CAIDs.Count(); i++) {
        if (i) ss << ",";
        ss << c.CAIDs[i];
        }
     }
  else ss << "0";

  ss << std::dec << ":" << c.SID << ":" << c.ONID << ":" << c.TID << ":" << c.RID;
  return ss.str();
}

/*******************************************************************************
 * VDR output, see http://www.tvdr.de/
 ******************************************************************************/

void PrintVDR(std::vector<TChannel>& List) {
  for(auto c:UniqueChannels(List))
     OutputLine(VdrChannel(c));
}

/*******************************************************************************
 * tzap/czap/xine output
 ******************************************************************************/

void PrintXine(std::vector<TChannel>& List, bool mplayer) {
  if (List.size() < 1) return;

  std::stringstream ss;

  for(auto c:UniqueChannels(List)) {
     if (c.Name.empty())
        ss << "???";
     else
        ss << c.Name;

     if (c.Provider.size())
        ss << '(' << c.Provider << "):";
     else
        ss << ':';

     uint32_t freq_Hz = c.Frequency;
     while(freq_Hz && (freq_Hz < 1000000)) freq_Hz *= 1000;

     if (c.Source    == "A") {
        ss << freq_Hz << ':';
        if (c.Modulation == 10)         ss << "VSB_8";
        else if (c.Modulation == 11)    ss << "VSB_16";
        else if (c.Modulation == 256)   ss << "QAM_256";
        else                            ss << "QAM_AUTO";
        }
     else if (c.Source    == "C") {
        ss << freq_Hz << ':';
        if (c.Inversion == 0)           ss << "INVERSION_OFF";
        else if (c.Inversion == 1)      ss << "INVERSION_ON";
        else                            ss << "INVERSION_AUTO";
        ss << ':' << c.Symbolrate << ':' << "FEC_NONE" << ':';

        if (c.Modulation == 32)         ss << "QAM_32";
        else if (c.Modulation == 64)    ss << "QAM_64";
        else if (c.Modulation == 128)   ss << "QAM_128";
        else if (c.Modulation == 256)   ss << "QAM_256";
        else                            ss << "QAM_AUTO";
        }
     else if (c.Source[0] == 'S') {
        ss << freq_Hz/1000 << ':';
        ss << (char) std::tolower(c.Polarization) << ':';

        if (DiseqcRotorPosition() != -1)
           ss << DiseqcRotorPosition() << ':';
        else
           ss << 0 << ':';
        ss << c.Symbolrate; 
        }
     else if (c.Source    == "T") {
        ss << freq_Hz << ':';
        if (c.Inversion == 0)           ss << "INVERSION_OFF";
        else if (c.Inversion == 1)      ss << "INVERSION_ON";
        else                            ss << "INVERSION_AUTO";
        ss << ':';
        if (c.Bandwidth == 8)           ss << "BANDWIDTH_8_MHZ";
        else if (c.Bandwidth == 7)      ss << "BANDWIDTH_7_MHZ";
        else if (c.Bandwidth == 6)      ss << "BANDWIDTH_6_MHZ";
        else if (c.Bandwidth == 5)      ss << "BANDWIDTH_5_MHZ";
        else if (c.Bandwidth == 10)     ss << "BANDWIDTH_10_MHZ";
        else if (c.Bandwidth == 1712)   ss << "BANDWIDTH_1_712_MHZ";
        else                            ss << "BANDWIDTH_AUTO";
        ss << ':';
	     if (c.FEC == 12)                ss << "FEC_1_2";
	     else if (c.FEC == 23)           ss << "FEC_2_3";
	     else if (c.FEC == 34)           ss << "FEC_3_4";
	     else if (c.FEC == 35)           ss << "FEC_3_5";
	     else if (c.FEC == 45)           ss << "FEC_4_5";
	     else if (c.FEC == 56)           ss << "FEC_5_6";
        else                            ss << "FEC_AUTO";
        ss << ":FEC_AUTO:";
        if (c.Modulation == 2)          ss << "QPSK";
        else if (c.Modulation == 16)    ss << "QAM_16";
        else if (c.Modulation == 64)    ss << "QAM_64";
        else if (c.Modulation == 256)   ss << "QAM_256";
        else                            ss << "QAM_AUTO";
        ss << ':';
        if (c.Transmission == 1)        ss << "TRANSMISSION_MODE_1K";
        else if (c.Transmission == 2)   ss << "TRANSMISSION_MODE_2K";
        else if (c.Transmission == 4)   ss << "TRANSMISSION_MODE_4K";
        else if (c.Transmission == 8)   ss << "TRANSMISSION_MODE_8K";
        else if (c.Transmission == 16)  ss << "TRANSMISSION_MODE_16K";
        else if (c.Transmission == 32)  ss << "TRANSMISSION_MODE_32K";
        else                            ss << "TRANSMISSION_MODE_AUTO";
        ss << ':';
        if (c.Guard == 4)               ss << "GUARD_INTERVAL_1_4";
        else if (c.Guard == 8)          ss << "GUARD_INTERVAL_1_8";
        else if (c.Guard == 16)         ss << "GUARD_INTERVAL_1_16";
        else if (c.Guard == 32)         ss << "GUARD_INTERVAL_1_32";
        else if (c.Guard == 128)        ss << "GUARD_INTERVAL_1_128";
        else if (c.Guard == 19128)      ss << "GUARD_INTERVAL_19_128";
        else if (c.Guard == 19256)      ss << "GUARD_INTERVAL_19_256";
        else                            ss << "GUARD_INTERVAL_AUTO";
        ss << ':';
        if (c.Hierarchy == 0)           ss << "HIERARCHY_NONE";
        else if (c.Hierarchy == 1)      ss << "HIERARCHY_1";
        else if (c.Hierarchy == 2)      ss << "HIERARCHY_2";
        else if (c.Hierarchy == 4)      ss << "HIERARCHY_4";
        else                            ss << "HIERARCHY_AUTO";
        }
     ss << ':';
     ss << c.VPID.PID << ':';
     if (not mplayer) {
        if      (c.DPIDs.Count())       ss << c.DPIDs[0].PID;
        else if (c.APIDs.Count())       ss << c.APIDs[0].PID;
        else                            ss << '0';
        }
     else {
        std::string pids;
        for(int i=0; i<c.DPIDs.Count(); i++) {
           if (pids.size()) pids += '+';
           pids += std::to_string(c.DPIDs[i].PID);
           }
        for(int i=0; i<c.APIDs.Count(); i++) {
           if (pids.size()) pids += '+';
           pids += std::to_string(c.APIDs[i].PID);
           }
        if (pids.empty()) ss << '0';
        else              ss << pids;          
        }
     ss << ':';
     ss << c.SID << std::endl;
     }
  OutputLine(ss.str());
}

/*******************************************************************************
 * wirbelscan satellites.dat output.
 ******************************************************************************/

void PrintSatellitesDat(std::vector<TChannel>& List) {
  if (List.size() < 1) return;

  auto SatId = [](std::string Source, std::string& Position, std::string& Orbital, std::string& Id) {
     Position = Source.substr(1,32);
     int Pos = (0.5f + 10.0f * std::stof(Position));
     Id = Source;
     size_t dot = Id.find('.');
     if (dot == std::string::npos)
        Id += "0";
     else {
        Id[dot] = Id.back();
        Id.pop_back();
        }     
     if (Source.back() == 'E') Orbital = std::to_string(Pos);
     else                      Orbital = std::to_string(3600 - Pos);
     while(Orbital.size() < 4) Orbital.insert(0, "0");
     };

  std::stringstream ss;
  auto list = UniqueTransponders(List);
  std::string Position, Orbital, Id, s;

  SatId(List[0].Source, Position, Orbital, Id);

  ss << "/*******************************************************************************\n";
  ss << " * ident  : " + SatNames[WirbelscanSetup.SatIndex] + "\n";
  ss << " * orbital: " + Orbital + "\n";
  ss << " * sat_pos: " + Position + "\n";
  ss << " *******************************************************************************/\n"; 
  ss << "B(__" << Id << ")\n";

  for(auto t:list) {
     if (t.DelSys > 0)
        ss << "{6 , ";
     else
        ss << "{5 , ";

     ss << FrontFill(IntToStr(t.Frequency), 5) << ", ";
 
     if (t.Polarization == 'H')       ss << "0";
     else if (t.Polarization == 'V')  ss << "1";
     else if (t.Polarization == 'L')  ss << "2";
     else if (t.Polarization == 'R')  ss << "3";
     ss << ", ";

     ss << FrontFill(IntToStr(t.Symbolrate), 5) << ", "; 

     if (t.FEC == 0)        ss << FrontFill("0" ,2);
     else if (t.FEC == 12)  ss << FrontFill("1" ,2);
     else if (t.FEC == 23)  ss << FrontFill("2" ,2);
     else if (t.FEC == 34)  ss << FrontFill("3" ,2);
     else if (t.FEC == 45)  ss << FrontFill("4" ,2);
     else if (t.FEC == 56)  ss << FrontFill("5" ,2);
     else if (t.FEC == 67)  ss << FrontFill("6" ,2);
     else if (t.FEC == 78)  ss << FrontFill("7" ,2);
     else if (t.FEC == 89)  ss << FrontFill("8" ,2);
     else if (t.FEC == 35)  ss << FrontFill("10",2);
     else if (t.FEC == 910) ss << FrontFill("11",2);
     else                   ss << FrontFill("9" ,2);
     ss << ", ";

     if (t.Rolloff == 35)      ss << "0";
     else if (t.Rolloff == 20) ss << "1";
     else if (t.Rolloff == 25) ss << "2";
     else                      ss << "3";
     ss << ", ";

     if (t.Modulation == 2)      ss << FrontFill("0" ,2);
     else if (t.Modulation == 5) ss << FrontFill("9" ,2);
     else if (t.Modulation == 6) ss << FrontFill("10",2);
     else if (t.Modulation == 7) ss << FrontFill("11",2);
     else                        ss << FrontFill("6" ,2);
     ss << ", ";

     ss << FrontFill(IntToStr(t.StreamId),3);

     ss << "},          // "
        << "NID = " << std::to_string(t.NID)
        << ", "
        << "TID = " << std::to_string(t.TID)
        << "\n";
     }
  ss << "E(__" << Id << ")\n"; 
  OutputLine(ss.str());
}


/*******************************************************************************
 * ini transponder output. 
 ******************************************************************************/

void PrintIni(std::vector<TChannel>& List) {
  if (List.size() < 1) return;

  std::stringstream ss;
  auto list = UniqueTransponders(List);
  auto t = time(nullptr);
  auto tm = localtime(&t);
  auto year  = IntToStr(1900 + tm->tm_year, 4, false, '0');
  auto month = IntToStr(1    + tm->tm_mon , 2, false, '0');
  auto day   = IntToStr(0    + tm->tm_mday, 2, false, '0');


  auto SatId = [](std::string Source) -> std::string {
     std::string result;
     int Pos = (0.5f + 10.0f * std::stof(Source.substr(1,32)));
     if (Source.back() == 'E') result = std::to_string(Pos);
     else                      result = std::to_string(3600 - Pos);
     while(result.size() < 4)  result.insert(0, "0");
     return result;
     };

  ss << ";------------------------------------------------------------------------------\n";
  ss << "; file automatically generated by w_scan_cpp\n";
  ss << "; https://www.gen2vdr.de/wirbel/w_scan_cpp/index2.html\n";
  ss << ";------------------------------------------------------------------------------\n";
  ss << "; location and provider: \n";
  ss << "; date (yyyy-mm-dd)    : " + year + "-" + month + "-" + day + "\n";
  ss << "; provided by (opt)    : --wirbel--\n\n";
  ss << "[SATTYPE]" << std::endl;

  if (list[0].Source == "A") {
     ss << "1=6000" << std::endl;
     ss << "2=ATSC" << std::endl;
     }
  else if (list[0].Source == "C") {
     ss << "1=4000" << std::endl;
     ss << "2=Cable" << std::endl;
     }
  else if (list[0].Source == "T") {
     ss << "1=5000" << std::endl;
     ss << "2=Terr" << std::endl;
     }
  else if (list[0].Source[0] == 'S') {
     ss << "1=" << SatId(list[0].Source) << std::endl;
     ss << "2=" << list[0].Source.substr(1,8) << std::endl;
     }
  ss << std::endl << "[DVB]" << std::endl;
  ss << "0=" << list.size() << std::endl;

  size_t n = 0;
  for(auto c:list) {
     ss << n++ << "=";

     if (c.Source == "A") {
        uint32_t freq_Hz = c.Frequency;
        while(freq_Hz && (freq_Hz < 1000000)) freq_Hz *= 1000;
        ss << c.Frequency/1000 << ',';
        ss << 6 << std::endl;
        }
     else if (c.Source == "C") {
        uint32_t freq_Hz = c.Frequency;
        while(freq_Hz && (freq_Hz < 1000000)) freq_Hz *= 1000;
        ss << c.Frequency/1000 << ',';
        if (c.Modulation == 64)
           ss << 3;
        else if (c.Modulation == 128)
           ss << 4;
        else if (c.Modulation == 256)
           ss << 5;
        ss << ',';

        }
     else if (c.Source == "T") {
        uint32_t freq_Hz = c.Frequency;
        while(freq_Hz && (freq_Hz < 1000000)) freq_Hz *= 1000;
        ss << c.Frequency/1000 << ',';
        ss << c.Bandwidth << std::endl;
        }
     else if (c.Source[0] == 'S') {
        ss << c.Frequency    << ',';
        ss << c.Polarization << ',';
        ss << c.Symbolrate   << ',';
        if (c.FEC != 999)
           ss << c.FEC;
        if (c.DelSys) {
           ss << ",S2";
           if (c.Modulation == 2)       ss << ";QPSK";
           else if (c.Modulation == 5)  ss << ";8PSK";
	        else if (c.Modulation == 6)  ss << ";16APSK";
	        else if (c.Modulation == 7)  ss << ";32APSK";
           else if (c.Modulation == 12) ss << ";DQPSK";
           }
        ss << std::endl;
        }
     }

  OutputLine(ss.str());
}


/*******************************************************************************
 * channel.dat output
 * Non-documented and proprietary binary format, not all bytes known.
 * Some fields guessed.
 ******************************************************************************/

void PrintDat(std::vector<TChannel>& List, std::string BinaryFile) {
  std::ofstream dat(BinaryFile.c_str(), std::ofstream::out | std::ofstream::binary);

  if (not dat.is_open()) {
     ErrorMessage("Error writing to " + BinaryFile);
     return;
     }

  auto PutByte = [](uint8_t v, uint8_t*& p) {
     *p++ = v;
     };

  auto PutWord = [](uint16_t v, uint8_t*& p) {
     *p++ = v; v >>= 8;
     *p++ = v;
     };

  auto PutDword = [](uint32_t v, uint8_t*& p) {
     *p++ = v; v >>= 8;
     *p++ = v; v >>= 8;
     *p++ = v; v >>= 8;
     *p++ = v;
     };

  auto PutString = [](std::string s, uint8_t*& p, size_t bufsz) {
     *p++ = s.size();
     for(size_t i=0; i<bufsz; i++) {
        if (i<s.size()) *p++ = (uint8_t) s[i];
        else            *p++ = 0;
        }
     };

  const unsigned char header[] = {0x04, 0x42, 0x32, 0x43, 0x32, 0x01, 0x08};
  dat.write((const char*) header, 7);

  for(auto c:UniqueChannels(List)) {
     uint8_t buf[256], *p = buf;
     uint8_t tune_flags, stream_flags;

     if      (c.Source    == "A") PutByte(3,p);
     else if (c.Source    == "C") PutByte(0,p);
     else if (c.Source[0] == 'S') PutByte(1,p);
     else if (c.Source    == "T") PutByte(2,p);

     PutByte(0,p);

     PutByte(c.DelSys,p);

     stream_flags = 0;
     if (c.CAIDs.Count())  stream_flags |= 1;
     if (c.TPID == 0)      stream_flags |= 2;
     if (c.VPID.PID)       stream_flags |= 8;
     if (c.APIDs.Count())  stream_flags |= 16;
     if (c.DPIDs.Count())  stream_flags |= 16;
     PutByte(stream_flags,p);

     if (c.Source[0] == 'S')
        PutDword(c.Frequency,p);
     else {
        uint32_t freq_Hz = c.Frequency;
        while(freq_Hz && (freq_Hz < 1000000)) freq_Hz *= 1000;
        PutDword(freq_Hz/1000, p);
        }

     if ((c.Source[0] == 'S') or (c.Source == "C"))
        PutDword(c.Symbolrate,p);
     else
        PutDword(0,p);

     if ((c.Source[0] == 'S') and Setup.LnbSLOF) {
        if (c.Frequency >= Setup.LnbSLOF)
           PutWord(Setup.LnbFrequHi,p);
        else
           PutWord(Setup.LnbFrequLo,p);
        }
     else
        PutWord(0,p);

     PutWord(c.PMT,p);

     PutWord(0,p);

	  if (c.Modulation == 2)             tune_flags  = 1;
	  else if (c.Modulation == 5)        tune_flags  = 2;
	  else if (c.Modulation == 6)        tune_flags  = 3;
     else                               tune_flags  = 0;
     if (c.DelSys)                      tune_flags |= 4;
     if (c.Rolloff == 25)               tune_flags |= 8;
     else if (c.Rolloff == 20)          tune_flags |= 16;
     if (c.Inversion == 999)            tune_flags |= 32;
     else if (c.Inversion == 0)         tune_flags |= 64;
     else if (c.Inversion == 1)         tune_flags |= 96;
     if (c.Pilot == 1)                  tune_flags |= 128;
     PutByte(tune_flags,p);

     stream_flags = 0;
     if (c.DPIDs.Count())               stream_flags |= 1;
     if (c.VPID.Type == 27)             stream_flags |= 16;
     else if (c.VPID.Type == 36)        stream_flags |= 32;
     PutByte(stream_flags,p);

	  if      (c.FEC == 12)              PutByte(1,p);
	  else if (c.FEC == 23)              PutByte(2,p);
	  else if (c.FEC == 34)              PutByte(3,p);
	  else if (c.FEC == 45)              PutByte(8,p);
	  else if (c.FEC == 56)              PutByte(4,p);
	  else if (c.FEC == 78)              PutByte(5,p);
	  else if (c.FEC == 89)              PutByte(6,p);
	  else if (c.FEC == 35)              PutByte(7,p);
	  else if (c.FEC == 910)             PutByte(9,p);
     else                               PutByte(0,p);
     PutByte(0,p);

     PutWord(0,p);
                                        
     if (c.Source    == "C") {
	     if      (c.Modulation == 32)    PutByte(2,p);
	     else if (c.Modulation == 64)    PutByte(3,p);
	     else if (c.Modulation == 128)   PutByte(4,p);
	     else if (c.Modulation == 256)   PutByte(5,p);
        else                            PutByte(0,p);
        }
     else if (c.Source[0] == 'S') {
        if      (c.Polarization == 'H') PutByte(0,p);
        else if (c.Polarization == 'V') PutByte(1,p);
        else if (c.Polarization == 'L') PutByte(2,p);
        else if (c.Polarization == 'R') PutByte(3,p);
        }
     else if (c.Source    == "T") {
        if (c.Bandwidth == 7)           PutByte(1,p);
        else if (c.Bandwidth == 8)      PutByte(2,p);
        else                            PutByte(0,p);
        }
     else
        PutByte(0,p);

     PutByte(0,p);

     PutWord(0,p);

     if ((c.Source[0] == 'S') and (Setup.LnbSLOF) and (c.Frequency >= Setup.LnbSLOF))
        PutByte(1,p);
     else
        PutByte(0,p);

     PutByte(0,p);

     PutWord(0,p);

     PutByte(3,p); // <- fixme

     PutByte(0,p);

     PutWord(0,p);

     if      (c.DPIDs.Count())          PutWord(c.DPIDs[0].PID,p);
     else if (c.APIDs.Count())          PutWord(c.APIDs[0].PID,p);
     else                               PutWord(0,p);

     PutWord(0,p);

     PutWord(c.VPID.PID,p);

     PutWord(c.TID,p);

     PutWord(c.TPID,p);

     PutWord(c.ONID,p);

     PutWord(c.SID,p);

     PutWord(c.PCR,p);

     if      (c.Source    == "A") PutString("ATSC",p,25);
     else if (c.Source    == "C") PutString("Cable",p,25);
     else if (c.Source[0] == 'S') PutString(SatNames[WirbelscanSetup.SatIndex].c_str(),p,25);
     else if (c.Source    == "T") PutString("Terr",p,25);

     if (c.Name.size())           PutString(c.Name,p,25);
     else                         PutString("???",p,25);

     if (c.Provider.size())       PutString(c.Provider,p,25);
     else                         PutString("???",p,25);

     PutByte(tune_flags,p);

     PutByte(0,p);

     dat.write((const char*) buf, p-buf);
     }
  dat.close();
}


/*******************************************************************************
 * VLC output
 ******************************************************************************/
void XmlString(std::string& s) {
  if (s.empty()) return;
  // those are the five predefined XML char entities
  ReplaceAll(s, "&", "&amp;");
  ReplaceAll(s, "<", "&lt;");
  ReplaceAll(s, ">", "&gt;");
  ReplaceAll(s, "'", "&apos;");
  ReplaceAll(s, "\"", "&quot;");
}

void PrintVLC(std::vector<TChannel>& List) {
  std::stringstream ss;
  size_t indent = 0;

  ss << INDENT << "<?xml"
     << " version="  << '"' << "1.0"   << '"'
     << " encoding=" << '"' << "UTF-8" << '"'
     << "?>" << std::endl;

  ss << INDENT << "<playlist"
     << " version="   << '"' << "1" << '"'
     << " xmlns="     << '"' << "http://xspf.org/ns/0/" << '"'
     << " xmlns:vlc=" << '"' << "http://www.videolan.org/vlc/playlist/ns/0/" << '"'
     << '>' << std::endl;
  indent++;

  ss << INDENT << "<title>DVB Playlist</title>" << std::endl;
  ss << INDENT << "<creator>w_scan_cpp</creator>" << std::endl;
  ss << INDENT << "<info>https://gen2vdr.de/wirbel/w_scan_cpp/index2.html</info>" << std::endl;
  ss << INDENT << "<trackList>" << std::endl;
  indent++;

  const char* AppUrl = "http://www.videolan.org/vlc/playlist/0";
  size_t TitleNumber = 1;

  for(auto c:UniqueChannels(List)) {
     ss << INDENT << "<track>" << std::endl;
     indent++;

     if (c.LCN != -1)
        TitleNumber = c.LCN;
     ss << INDENT << "<trackNum>" << IntToStr(TitleNumber);
     if (c.Source == "A" and c.LCN_minor != -1)
        ss << "." << c.LCN_minor;
     ss << "</trackNum>" << std::endl;

     std::string title;
     if (not c.Name.empty()) {
        title = c.Name;
        XmlString(title);
        }
     else
        title = "Channel";
     ss << INDENT << "<title>" << title << "</title>" << std::endl;

     uint64_t freq_Hz = c.Frequency;
     auto freq_Min = [](std::string Source) -> uint64_t {
        return (Source.find('S') == 0) ? 3000000000ULL : 50000000ULL;
        };

     while(freq_Hz && (freq_Hz < freq_Min(c.Source))) freq_Hz *= 1000;

     if (c.Source == "A") {
        ss << INDENT << "<location>atsc://frequency=" << freq_Hz << "</location>" << std::endl;

        ss << INDENT << "<extension application=" << '"' << AppUrl << '"' << ">" << std::endl;
        indent++;

        ss << INDENT << "<vlc:id>" << TitleNumber;
        if (c.LCN_minor != -1)
           ss << "." << c.LCN_minor;
        ss << "</vlc:id>" << std::endl;

        if (c.LCN != -1) {
           ss << INDENT << "<vlc:option>"
              << "logical-channel-number=" << c.LCN
              << "</vlc:option>" << std::endl;
           if (c.LCN_minor != -1)
              ss << INDENT << "<vlc:option>"
                 << "logical-channel-number-minor=" << c.LCN_minor
                 << "</vlc:option>" << std::endl;
           }

        ss << INDENT << "<vlc:option>" << "dvb-ts-id=" << c.TID << "</vlc:option>" << std::endl;
        ss << INDENT << "<vlc:option>" << "program="   << c.SID << "</vlc:option>" << std::endl;

        if (c.Modulation != 999) {
           ss << INDENT << "<vlc:option>dvb-modulation=";
	        if (c.Modulation == 10)         ss << "8VSB";
	        else if (c.Modulation == 11)    ss << "16VSB";
	        else if (c.Modulation == 64)    ss << "64QAM"; // may be unused at all.
	        else if (c.Modulation == 256)   ss << "256QAM";
           else                            ss << "AUTO";
           ss << "</vlc:option>" << std::endl;
           }

        indent--;
        ss << INDENT << "</extension>" << std::endl;
        }
     if (c.Source == "C") {
        ss << INDENT << "<location>dvb-c://frequency=" << freq_Hz << "</location>" << std::endl;

        ss << INDENT << "<extension" << " application=" << '"' << AppUrl << '"' << ">" << std::endl;
        indent++;

        if (c.LCN != -1)
           ss << INDENT << "<vlc:option>" << "logical-channel-number=" << c.LCN << "</vlc:option>" << std::endl;

        ss << INDENT << "<vlc:id>"     << TitleNumber  << "</vlc:id>" << std::endl;
        ss << INDENT << "<vlc:option>" << "dvb-ts-id=" << c.TID             << "</vlc:option>" << std::endl;
        ss << INDENT << "<vlc:option>" << "program="   << c.SID << "</vlc:option>" << std::endl;
        ss << INDENT << "<vlc:option>" << "dvb-srate=" << c.Symbolrate*1000 << "</vlc:option>" << std::endl;

        if (c.Modulation != 999) {
           ss << INDENT << "<vlc:option>" << "dvb-modulation=";
	        if (c.Modulation == 32)         ss << "32QAM";
	        else if (c.Modulation == 64)    ss << "64QAM";
	        else if (c.Modulation == 128)   ss << "128QAM";
	        else if (c.Modulation == 256)   ss << "256QAM";
           else                            ss << "AUTO";
           ss << "</vlc:option>" << std::endl;
           }
        if (c.Inversion != 999) {
           ss << INDENT << "<vlc:option>" << "dvb-inversion=" << c.Inversion << "</vlc:option>" << std::endl;
           }

        indent--;
        ss << INDENT << "</extension>" << std::endl;
        }
     if (c.Source == "T") {
        ss << INDENT << "<location>dvb-t";
        if (c.DelSys == 1) ss << "2";
        ss << "://frequency=" << freq_Hz << "</location>" << std::endl;

        ss << INDENT << "<extension" << " application=" << '"' << AppUrl << '"' << ">" << std::endl;
        indent++;

        if (c.LCN != -1)
           ss << INDENT << "<vlc:option>" << "logical-channel-number=" << c.LCN << "</vlc:option>" << std::endl;

        int bw = c.Bandwidth;
        while(bw > 999) bw = round(bw/1000.0);

        ss << INDENT << "<vlc:id>"     << TitleNumber      << "</vlc:id>" << std::endl;
        ss << INDENT << "<vlc:option>" << "dvb-ts-id="     << c.TID << "</vlc:option>" << std::endl;
        ss << INDENT << "<vlc:option>" << "program="       << c.SID << "</vlc:option>" << std::endl;
        ss << INDENT << "<vlc:option>" << "dvb-bandwidth=" << bw    << "</vlc:option>" << std::endl;

        if (c.Inversion    != 999)
           ss << INDENT << "<vlc:option>dvb-inversion=" << c.Inversion << "</vlc:option>" << std::endl;
        if (c.FEC          != 999) {
           ss << INDENT << "<vlc:option>dvb-code-rate-hp=";
	        if (c.FEC == 12)         ss << "1/2";
	        else if (c.FEC == 23)    ss << "2/3";
	        else if (c.FEC == 34)    ss << "3/4";
	        else if (c.FEC == 45)    ss << "4/5";
	        else if (c.FEC == 56)    ss << "5/6";
	        else if (c.FEC == 67)    ss << "6/7";
	        else if (c.FEC == 78)    ss << "7/8";
	        else if (c.FEC == 35)    ss << "3/5";
	        else if (c.FEC == 89)    ss << "8/9";
	        else if (c.FEC == 910)   ss << "9/10";
	        else if (c.FEC == 25)    ss << "2/5";
	        else if (c.FEC == 14)    ss << "1/4";
	        else if (c.FEC == 13)    ss << "1/3";
           else                     ss << "-1";
           ss << "</vlc:option>" << std::endl;
           }
        if (c.Modulation != 999) {
           ss << INDENT << "<vlc:option>" << "dvb-modulation=";
	        if (c.Modulation == 2)        ss << "QPSK";
	        else if (c.Modulation == 16)  ss << "16QAM";
	        else if (c.Modulation == 64)  ss << "64QAM";
	        else if (c.Modulation == 256) ss << "256QAM";
           else                          ss << "AUTO";
           ss << "</vlc:option>" << std::endl;
           }
        if (c.Transmission != 999)
           ss << INDENT << "<vlc:option>" << "dvb-transmission=" << c.Transmission << "</vlc:option>" << std::endl;
        if (c.Guard != 999) {
           ss << INDENT << "<vlc:option>" << "dvb-guard=";
	        if (c.Guard == 4)             ss << "1/4";
	        else if (c.Guard == 8)        ss << "1/8";
	        else if (c.Guard == 16)       ss << "1/16";
	        else if (c.Guard == 32)       ss << "1/32";
	        else if (c.Guard == 128)      ss << "1/128";
	        else if (c.Guard == 19128)    ss << "19/128";
	        else if (c.Guard == 19256)    ss << "19/256";
           else                          ss << "AUTO";
           ss << "</vlc:option>" << std::endl;
           }
        if (c.DelSys == 1) {
           if (c.StreamId  != 999)
              ss << INDENT << "<vlc:option>" << "dvb-plp-id=" << c.StreamId << "</vlc:option>" << std::endl;
           }
        indent--;
        ss << INDENT << "</extension>" << std::endl;
        }
     if (c.Source.find('S') == 0) {
        ss << INDENT << "<location>dvb-s";
        if (c.DelSys == 1) ss << "2";
        ss << "://frequency=" << freq_Hz << "</location>" << std::endl;

        ss << INDENT << "<extension" << " application=" << '"' << AppUrl << '"' << ">" << std::endl;
        indent++;

        if (c.LCN != -1)
           ss << INDENT << "<vlc:option>" << "logical-channel-number=" << c.LCN << "</vlc:option>" << std::endl;

        ss << INDENT << "<vlc:id>"     << TitleNumber         << "</vlc:id>" << std::endl;
        ss << INDENT << "<vlc:option>" << "dvb-ts-id="        << c.TID << "</vlc:option>" << std::endl;
        ss << INDENT << "<vlc:option>" << "program="          << c.SID << "</vlc:option>" << std::endl;
        ss << INDENT << "<vlc:option>" << "dvb-polarization=" << c.Polarization << "</vlc:option>" << std::endl;
        ss << INDENT << "<vlc:option>" << "dvb-srate="        << c.Symbolrate*1000 << "</vlc:option>" << std::endl;
        if (c.Inversion    != 999)
           ss << INDENT << "<vlc:option>dvb-inversion=" << c.Inversion << "</vlc:option>" << std::endl;
        if (c.FEC          != 999) {
           ss << INDENT << "<vlc:option>dvb-fec=";
	        if (c.FEC == 12)       ss << "1/2";
	        else if (c.FEC == 23)  ss << "2/3";
	        else if (c.FEC == 34)  ss << "3/4";
	        else if (c.FEC == 45)  ss << "4/5";
	        else if (c.FEC == 56)  ss << "5/6";
	        else if (c.FEC == 67)  ss << "6/7";
	        else if (c.FEC == 78)  ss << "7/8";
	        else if (c.FEC == 89)  ss << "8/9";
	        else if (c.FEC == 35)  ss << "3/5";
	        else if (c.FEC == 910) ss << "9/10";
	        else if (c.FEC == 25)  ss << "2/5";
	        else if (c.FEC == 14)  ss << "1/4";
	        else if (c.FEC == 13)  ss << "1/3";
           else                   ss << "-1";
           ss << "</vlc:option>" << std::endl;
           }
        if (c.DelSys == 1) {
           if (c.StreamId  != 999)
              ss << INDENT << "<vlc:option>" << "dvb-plp-id=" << c.StreamId << "</vlc:option>" << std::endl;
           ss << INDENT << "<vlc:option>" << "dvb-modulation=";
	        if (c.Modulation == 2)       ss << "QPSK";
	        else if (c.Modulation == 5)  ss << "8PSK";
	        else if (c.Modulation == 6)  ss << "16APSK";
	        else if (c.Modulation == 7)  ss << "32APSK";
           else if (c.Modulation == 12) ss << "DQPSK";
           else                         ss << "AUTO";
           ss << "</vlc:option>" << std::endl;

           ss << INDENT << "<vlc:option>" << "dvb-rolloff=";
           if (c.Rolloff == 20)         ss << 20;
           else if (c.Rolloff == 25)    ss << 25;
           else                         ss << 35;
           ss << "</vlc:option>" << std::endl;
           }

        int h,l,s;
        GetLnb(l, h, s);
        ss << INDENT << "<vlc:option>" << "dvb-lnb-low="    << l << "</vlc:option>" << std::endl;
        ss << INDENT << "<vlc:option>" << "dvb-lnb-high="   << h << "</vlc:option>" << std::endl;
        ss << INDENT << "<vlc:option>" << "dvb-lnb-switch=" << s << "</vlc:option>" << std::endl;

        if (DiseqcSwitchPosition() != -1)
           ss << INDENT << "<vlc:option>" << "dvb-satno=" << DiseqcSwitchPosition() << "</vlc:option>" << std::endl;

        indent--;
        ss << INDENT << "</extension>" << std::endl;
        }

     TitleNumber++;
     indent--;
     ss << INDENT << "</track>" << std::endl;
     } // end for()

  indent--;
  ss << INDENT << "</trackList>" << std::endl;

  indent--;
  ss << INDENT << "</playlist>" << std::endl;

  OutputLine(ss.str());
}


/*******************************************************************************
 * VLC satip output
 ******************************************************************************/

void PrintVLCsatip(std::vector<TChannel>& List) {
  std::stringstream ss;
  size_t indent = 0;

  ss << INDENT << "<?xml"
     << " version="  << '"' << "1.0"   << '"'
     << " encoding=" << '"' << "UTF-8" << '"'
     << "?>" << std::endl;

  ss << INDENT << "<playlist"
     << " version="   << '"' << "1" << '"'
     << " xmlns="     << '"' << "http://xspf.org/ns/0/" << '"'
     << " xmlns:vlc=" << '"' << "http://www.videolan.org/vlc/playlist/ns/0/" << '"'
     << '>' << std::endl;
  indent++;

  ss << INDENT << "<title>DVB Playlist</title>" << std::endl;
  ss << INDENT << "<creator>w_scan_cpp</creator>" << std::endl;
  ss << INDENT << "<info>https://gen2vdr.de/wirbel/w_scan_cpp/index2.html</info>" << std::endl;
  ss << INDENT << "<trackList>" << std::endl;
  indent++;

  const char* AppUrl = "http://www.videolan.org/vlc/playlist/0";
  size_t TitleNumber = 1;

  for(auto c:UniqueChannels(List)) {
     ss << INDENT << "<track>" << std::endl;
     indent++;

     if (c.LCN != -1)
        TitleNumber = c.LCN;
     ss << INDENT << "<trackNum>" << IntToStr(TitleNumber);
     if (c.Source == "A" and c.LCN_minor != -1)
        ss << "." << c.LCN_minor;
     ss << "</trackNum>" << std::endl;

     std::string title;
     if (not c.Name.empty()) {
        title = c.Name;
        XmlString(title);
        }
     else
        title += ". Channel";
     ss << INDENT << "<title>" << title << "</title>" << std::endl;

     uint32_t freq_Hz;
     double freq_MHz;

     freq_Hz = c.Frequency;
     freq_MHz = c.Frequency;     
     while(freq_Hz && (freq_Hz < 1000000)) freq_Hz *= 1000;
     while(freq_MHz && (freq_MHz > 1000.0)) freq_MHz /= 1000.0;

     ss << INDENT << "<location>rtsp://" << WirbelscanSetup.SatipAddr << "/?";

     if (c.Source == "A") {
        ErrorMessage("ATSC is not yet supported in SAT>IP");
        return;
        }
     if (c.Source == "C") {
        ss << "freq=" << freq_MHz;
        ss << "&amp;msys=dvbc";
        if (c.DelSys == 0) {
	        if (c.Modulation == 16)         ss << "&amp;mtype=16qam";
	        else if (c.Modulation == 32)    ss << "&amp;mtype=32qam";
	        else if (c.Modulation == 64)    ss << "&amp;mtype=64qam";
	        else if (c.Modulation == 128)   ss << "&amp;mtype=128qam";
	        else if (c.Modulation == 256)   ss << "&amp;mtype=256qam";

           ss << "&amp;sr=" << c.Symbolrate;

           if (c.Inversion != 999)
              ss << "&amp;specinv=" << c.Inversion;
           }
        else {
           ss << "2"; // DVB-C2

           int bw = c.Bandwidth;
           while(bw > 999) bw = round(bw/1000.0);

           if (bw != 999)
              ss << "&amp;bw=" << bw;
           if (c.StreamId != 999)
              ss << "&amp;plp=" << c.StreamId;
           }

        }
     if (c.Source == "T") {
        ss << "freq=" << freq_MHz;

        double bw = c.Bandwidth;
        while(bw >= 1000.0) bw /= 1000.0;
        ss << "&amp;bw=" << bw;

        if      (c.Transmission == 2)  ss << "&amp;tmode=2k";
        else if (c.Transmission == 4)  ss << "&amp;tmode=4k";
        else if (c.Transmission == 8)  ss << "&amp;tmode=8k";
        else if (c.Transmission == 1)  ss << "&amp;tmode=1k";
        else if (c.Transmission == 16) ss << "&amp;tmode=16k";
        else if (c.Transmission == 32) ss << "&amp;tmode=32k";

        if (c.Modulation == 2)        ss << "&amp;mtype=qpsk";
        else if (c.Modulation == 16)  ss << "&amp;mtype=16qam";
        else if (c.Modulation == 64)  ss << "&amp;mtype=64qam";
        else if (c.Modulation == 256) ss << "&amp;mtype=256qam";

        if (c.Guard == 4)             ss << "&amp;gi=14";
        else if (c.Guard == 8)        ss << "&amp;gi=18";
        else if (c.Guard == 16)       ss << "&amp;gi=116";
        else if (c.Guard == 32)       ss << "&amp;gi=132";
        else if (c.Guard == 128)      ss << "&amp;gi=1128";
        else if (c.Guard == 19128)    ss << "&amp;gi=19128";
        else if (c.Guard == 19256)    ss << "&amp;gi=19256";

        if (c.FEC == 12)         ss << "&amp;fec=12";
        else if (c.FEC == 35)    ss << "&amp;fec=35";
        else if (c.FEC == 23)    ss << "&amp;fec=23";
        else if (c.FEC == 34)    ss << "&amp;fec=34";
        else if (c.FEC == 45)    ss << "&amp;fec=45";
        else if (c.FEC == 56)    ss << "&amp;fec=56";
        else if (c.FEC == 78)    ss << "&amp;fec=78";

        if (c.DelSys == 0) {
           ss << "&amp;msys=dvbt";
           }
        else {
           ss << "&amp;msys=dvbt2";
           ss << "&amp;plp=" << c.StreamId;
           ss << "&amp;t2id=" << c.SystemId;
           ss << "&amp;sm=" << c.MISO;
           }        
        }
     if (c.Source.find('S') == 0) {
        ss << "src=1";
        ss << "&amp;freq=" << c.Frequency;
        ss << "&amp;pol=" << (char) std::tolower((unsigned char) c.Polarization);
        ss << "&amp;sr=" << c.Symbolrate;

        if (c.FEC == 12)       ss << "&amp;fec=12";
        else if (c.FEC == 23)  ss << "&amp;fec=23";
        else if (c.FEC == 34)  ss << "&amp;fec=34";
        else if (c.FEC == 56)  ss << "&amp;fec=56";
        else if (c.FEC == 78)  ss << "&amp;fec=78";
        else if (c.FEC == 89)  ss << "&amp;fec=89";
        else if (c.FEC == 35)  ss << "&amp;fec=35";
        else if (c.FEC == 45)  ss << "&amp;fec=45";
        else if (c.FEC == 910) ss << "&amp;fec=910";

        if (c.DelSys == 0) {
           ss << "&amp;ro=0.35&amp;msys=dvbs&amp;mtype=qpsk&amp;plts=off";
           }
        else {
           if (c.Rolloff == 20)      ss << "&amp;ro=0.20";
           else if (c.Rolloff == 25) ss << "&amp;ro=0.25";
           else                      ss << "&amp;ro=0.35";

           ss << "&amp;msys=dvbs2";

	        if (c.Modulation == 2)       ss << "&amp;mtype=qpsk";
	        else if (c.Modulation == 5)  ss << "&amp;mtype=8psk";

	        if (c.Pilot == 0) ss << "&amp;plts=off";
	        else              ss << "&amp;plts=on";
           }
        }

     ss << "&amp;pids=0,16,17,18";
     if (c.PMT > 0)           ss << ',' << c.PMT;
     if (c.VPID.PID > 0)      ss << ',' << c.VPID.PID;
     if (c.APIDs.Count() > 0) ss << ',' << c.APIDs[0].PID;
     if (c.DPIDs.Count() > 0) ss << ',' << c.DPIDs[0].PID;        
     if (c.TPID > 0)          ss << ',' << c.TPID;
     ss << "</location>" << std::endl;

     ss << INDENT << "<extension application=" << '"' << AppUrl << '"' << ">" << std::endl;
     indent++;

     ss << INDENT << "<vlc:id>" << TitleNumber;
     if (c.Source == "A" and c.LCN_minor != -1)
        ss << "." << c.LCN_minor;
     ss << "</vlc:id>" << std::endl;

     if (c.LCN != -1) {
         ss << INDENT << "<vlc:option>" << "logical-channel-number=" << c.LCN << "</vlc:option>" << std::endl;
         if (c.Source == "A" and c.LCN_minor != -1)
            ss << INDENT << "<vlc:option>" << "logical-channel-number-minor=" << c.LCN_minor << "</vlc:option>" << std::endl;
         }

     indent--;
     ss << INDENT << "</extension>" << std::endl;

     indent--;
     ss << INDENT << "</track>" << std::endl;
     TitleNumber++;
     } // end for()

  indent--;
  ss << INDENT << "</trackList>" << std::endl;

  indent--;
  ss << INDENT << "</playlist>" << std::endl;

  OutputLine(ss.str());
}


/*******************************************************************************
 * Initial Tuning Data for dvbv5_scan
 ******************************************************************************/

void PrintInitial(std::vector<TChannel>& List) {
  auto t = time(nullptr);
  auto tm = localtime(&t);
  auto year  = IntToStr(1900 + tm->tm_year, 4, false, '0');
  auto month = IntToStr(1    + tm->tm_mon , 2, false, '0');
  auto day   = IntToStr(0    + tm->tm_mday, 2, false, '0');

  OutputLine("#------------------------------------------------------------------------------");
  OutputLine("# file automatically generated by w_scan_cpp");
  OutputLine("# https://www.gen2vdr.de/wirbel/w_scan_cpp/index2.html");
  OutputLine("#------------------------------------------------------------------------------");
  OutputLine("# location and provider: ");
  OutputLine("# date (yyyy-mm-dd)    : " + year + "-" + month + "-" + day);
  OutputLine("# provided by (opt)    : --wirbel--\n\n");

  for(auto c:UniqueTransponders(List)) {
     std::stringstream ss;
     uint32_t freq_Hz;

     if (c.Provider.empty()) {
        if (c.Name.empty())
           c.Provider = "CHANNEL";
        else
           c.Provider = c.Name;
        }

     ss << "[" << c.Provider << "]" << std::endl;

     freq_Hz = c.Frequency;
     while(freq_Hz && (freq_Hz < 1000000)) freq_Hz *= 1000;

     if (c.Source == "A") {
	     if (c.Modulation == 10)         ss << "DELIVERY_SYSTEM = ATSC"          << std::endl;
        else if (c.Modulation == 11)    ss << "DELIVERY_SYSTEM = ATSC"          << std::endl;
        else                            ss << "DELIVERY_SYSTEM = DVBC/ANNEX_B"  << std::endl;

        ss << "FREQUENCY = " << freq_Hz << std::endl;

	     if (c.Modulation == 10)         ss << "MODULATION = VSB/8"              << std::endl;
	     else if (c.Modulation == 11)    ss << "MODULATION = VSB/16"             << std::endl;
	     else if (c.Modulation == 256)   ss << "MODULATION = QAM/256"            << std::endl;
        else                            ss << "MODULATION = QAM/AUTO"           << std::endl;

        ss << "INVERSION = AUTO" << std::endl;
        }
     else if (c.Source == "C") {
        ss << "DELIVERY_SYSTEM = DVBC/ANNEX_A"                                  << std::endl;
        ss << "FREQUENCY = " << freq_Hz                                         << std::endl;
        ss << "SYMBOL_RATE = " << c.Symbolrate*1000                             << std::endl;
        ss << "INNER_FEC = NONE"                                                << std::endl;

	     if (c.Modulation == 32)         ss << "MODULATION = QAM/32"             << std::endl;
	     else if (c.Modulation == 64)    ss << "MODULATION = QAM/64"             << std::endl;
	     else if (c.Modulation == 128)   ss << "MODULATION = QAM/128"            << std::endl;
	     else if (c.Modulation == 256)   ss << "MODULATION = QAM/256"            << std::endl;
        else                            ss << "MODULATION = QAM/AUTO"           << std::endl;

        ss << "INVERSION = AUTO" << std::endl;
        }
     else if (c.Source == "T") {
        size_t bHz = c.Bandwidth * 1000000;
        while(bHz > 10000000) bHz /= 1000;

	     if (c.DelSys == 0)              ss << "DELIVERY_SYSTEM = DVBT"          << std::endl;
        else                            ss << "DELIVERY_SYSTEM = DVBT2"         << std::endl;

        ss << "FREQUENCY = " << freq_Hz                                         << std::endl;
        ss << "BANDWIDTH_HZ = " << bHz                                          << std::endl;

        if (c.FEC == 0)                 ss << "CODE_RATE_HP = NONE"             << std::endl;
        else if (c.FEC == 12)           ss << "CODE_RATE_HP = 1/2"              << std::endl;
        else if (c.FEC == 23)           ss << "CODE_RATE_HP = 2/3"              << std::endl;
        else if (c.FEC == 34)           ss << "CODE_RATE_HP = 3/4"              << std::endl;
        else if (c.FEC == 35)           ss << "CODE_RATE_HP = 3/5"              << std::endl;
        else if (c.FEC == 45)           ss << "CODE_RATE_HP = 4/5"              << std::endl;
        else if (c.FEC == 56)           ss << "CODE_RATE_HP = 5/6"              << std::endl;
        else if (c.FEC == 67)           ss << "CODE_RATE_HP = 6/7"              << std::endl;
        else if (c.FEC == 78)           ss << "CODE_RATE_HP = 7/8"              << std::endl;
        else if (c.FEC == 89)           ss << "CODE_RATE_HP = 8/9"              << std::endl;
        else if (c.FEC == 910)          ss << "CODE_RATE_HP = 9/10"             << std::endl;
        else                            ss << "CODE_RATE_HP = AUTO"             << std::endl;

        if (c.FEC_low == 0)             ss << "CODE_RATE_LP = NONE"             << std::endl;
        else if (c.FEC_low == 12)       ss << "CODE_RATE_LP = 1/2"              << std::endl;
        else if (c.FEC_low == 23)       ss << "CODE_RATE_LP = 2/3"              << std::endl;
        else if (c.FEC_low == 34)       ss << "CODE_RATE_LP = 3/4"              << std::endl;
        else if (c.FEC_low == 35)       ss << "CODE_RATE_LP = 3/5"              << std::endl;
        else if (c.FEC_low == 45)       ss << "CODE_RATE_LP = 4/5"              << std::endl;
        else if (c.FEC_low == 56)       ss << "CODE_RATE_LP = 5/6"              << std::endl;
        else if (c.FEC_low == 67)       ss << "CODE_RATE_LP = 6/7"              << std::endl;
        else if (c.FEC_low == 78)       ss << "CODE_RATE_LP = 7/8"              << std::endl;
        else if (c.FEC_low == 89)       ss << "CODE_RATE_LP = 8/9"              << std::endl;
        else if (c.FEC_low == 910)      ss << "CODE_RATE_LP = 9/10"             << std::endl;
        else                            ss << "CODE_RATE_LP = AUTO"             << std::endl;

        if (c.Modulation == 2)          ss << "MODULATION = QPSK"               << std::endl;
        else if (c.Modulation == 16)    ss << "MODULATION = QAM/16"             << std::endl;
        else if (c.Modulation == 64)    ss << "MODULATION = QAM/64"             << std::endl;
        else if (c.Modulation == 256)   ss << "MODULATION = QAM/256"            << std::endl;
        else                            ss << "MODULATION = AUTO"               << std::endl;

        if (c.Transmission == 1)        ss << "TRANSMISSION_MODE = 1K"          << std::endl;
        else if (c.Transmission == 2)   ss << "TRANSMISSION_MODE = 2K"          << std::endl;
        else if (c.Transmission == 4)   ss << "TRANSMISSION_MODE = 4K"          << std::endl;
        else if (c.Transmission == 8)   ss << "TRANSMISSION_MODE = 8K"          << std::endl;
        else if (c.Transmission == 16)  ss << "TRANSMISSION_MODE = 16K"         << std::endl;
        else if (c.Transmission == 32)  ss << "TRANSMISSION_MODE = 32K"         << std::endl;
        else                            ss << "TRANSMISSION_MODE = AUTO"        << std::endl;

        if (c.Guard == 4)               ss << "GUARD_INTERVAL = 1/4"            << std::endl;
        else if (c.Guard == 8)          ss << "GUARD_INTERVAL = 1/8"            << std::endl;
        else if (c.Guard == 16)         ss << "GUARD_INTERVAL = 1/16"           << std::endl;
        else if (c.Guard == 32)         ss << "GUARD_INTERVAL = 1/32"           << std::endl;
        else if (c.Guard == 128)        ss << "GUARD_INTERVAL = 1/128"          << std::endl;
        else if (c.Guard == 19128)      ss << "GUARD_INTERVAL = 19/128"         << std::endl;
        else if (c.Guard == 19256)      ss << "GUARD_INTERVAL = 19/256"         << std::endl;
        else                            ss << "GUARD_INTERVAL = AUTO"           << std::endl;

        if (c.Hierarchy == 0)           ss << "HIERARCHY = NONE"                << std::endl;
        else if (c.Hierarchy == 1)      ss << "HIERARCHY = 1"                   << std::endl;
        else if (c.Hierarchy == 2)      ss << "HIERARCHY = 2"                   << std::endl;
        else if (c.Hierarchy == 4)      ss << "HIERARCHY = 4"                   << std::endl;
        else                            ss << "HIERARCHY = AUTO"                << std::endl;
        ss << "INVERSION = AUTO" << std::endl;
        if (c.DelSys == 1)
           if (c.StreamId  != 999)      ss << "STREAM_ID = " << c.StreamId      << std::endl;
        }
     else if (c.Source[0] == 'S') {
	     if (c.DelSys == 0)              ss << "DELIVERY_SYSTEM = DVBS"          << std::endl;
        else                            ss << "DELIVERY_SYSTEM = DVBS2"         << std::endl;

        ss << "FREQUENCY = " << c.Frequency * 1000                              << std::endl;

        if (c.Polarization == 'H')      ss << "POLARIZATION = HORIZONTAL"       << std::endl;
        else if (c.Polarization == 'V') ss << "POLARIZATION = VERTICAL"         << std::endl;
        else if (c.Polarization == 'L') ss << "POLARIZATION = LEFT"             << std::endl;
        else if (c.Polarization == 'R') ss << "POLARIZATION = RIGHT"            << std::endl;

        ss << "SYMBOL_RATE = " << c.Symbolrate*1000                             << std::endl;

        if (c.FEC == 0)                 ss << "INNER_FEC = NONE"                << std::endl;
        else if (c.FEC == 12)           ss << "INNER_FEC = 1/2"                 << std::endl;
        else if (c.FEC == 23)           ss << "INNER_FEC = 2/3"                 << std::endl;
        else if (c.FEC == 34)           ss << "INNER_FEC = 3/4"                 << std::endl;
        else if (c.FEC == 35)           ss << "INNER_FEC = 3/5"                 << std::endl;
        else if (c.FEC == 45)           ss << "INNER_FEC = 4/5"                 << std::endl;
        else if (c.FEC == 56)           ss << "INNER_FEC = 5/6"                 << std::endl;
        else if (c.FEC == 67)           ss << "INNER_FEC = 6/7"                 << std::endl;
        else if (c.FEC == 78)           ss << "INNER_FEC = 7/8"                 << std::endl;
        else if (c.FEC == 89)           ss << "INNER_FEC = 8/9"                 << std::endl;
        else if (c.FEC == 910)          ss << "INNER_FEC = 9/10"                << std::endl;
        else                            ss << "INNER_FEC = AUTO"                << std::endl;

        ss << "INVERSION = AUTO" << std::endl;

        if (c.DelSys == 1) {
	        if (c.Modulation == 2)       ss << "MODULATION = QPSK"               << std::endl;
	        else if (c.Modulation == 5)  ss << "MODULATION = PSK/8"              << std::endl;
	        else if (c.Modulation == 6)  ss << "MODULATION = APSK/16"            << std::endl;
	        else if (c.Modulation == 7)  ss << "MODULATION = APSK/32"            << std::endl;
           else                         ss << "MODULATION = QAM/AUTO"           << std::endl;

	        if (c.Pilot == 0)            ss << "PILOT = OFF"                     << std::endl;
	        else if (c.Pilot == 1)       ss << "PILOT = ON"                      << std::endl;
           else                         ss << "PILOT = AUTO"                    << std::endl;

	        if (c.Rolloff == 20)         ss << "ROLLOFF = 20"                    << std::endl;
	        else if (c.Rolloff == 25)    ss << "ROLLOFF = 25"                    << std::endl;
	        else if (c.Rolloff == 35)    ss << "ROLLOFF = 35"                    << std::endl;
           else                         ss << "ROLLOFF = AUTO"                  << std::endl;

           if (c.StreamId != 999)       ss << "STREAM_ID = " << c.StreamId      << std::endl;
           }
        }
     ss << std::endl;
     OutputLine(ss.str());
     }
  OutputLine("");
}


/*******************************************************************************
 * w_scan XML
 ******************************************************************************/

std::string as_double(double d) {
  std::stringstream ss;  
  if      (d >= 10000) ss << std::setprecision(8);
  else if (d >=  1000) ss << std::setprecision(6);
  else if (d >=   100) ss << std::setprecision(5);
  else if (d >=    10) ss << std::setprecision(4);
  else                 ss << std::setprecision(3);
  ss << d;
  return ss.str(); 
}

void PrintXML(std::vector<TChannel>& List) {
  std::stringstream ss;
  size_t indent = 0;

  ss << "<?xml version=" << '"' << "1.0" << '"' << "?>" << std::endl;
  ss << "<!DOCTYPE service_list SYSTEM " << '"'
     << "https://www.gen2vdr.de/wirbel/w_scan_cpp/service_list.dtd"
     << '"' << ">" << std::endl << std::endl;
  ss << "<!-- NOTE:" << std::endl;
  ss << "     if reading or writing w_scan XML file format:" << std::endl;
  ss << "        - please validate XML against DTD above." << std::endl;
  ss << "        - indent each XML element" << std::endl;
  ss << "        - indent using three spaces, don't use <TAB> char to indent." << std::endl;
  ss << "        - conform to requirements mentionend in DTD file." << std::endl;
  ss << " -->" << std::endl << std::endl;
  ss << INDENT << "<service_list>" << std::endl;

  indent++;
  ss << INDENT << "<transponders>" << std::endl;

  for(auto c:UniqueTransponders(List)) {
     indent++;
     ss << INDENT << "<transponder"
        << " ONID=" << '"' << c.ONID << '"'
        << " NID="  << '"' << c.NID  << '"'
        << " TSID=" << '"' << c.TID  << '"'
        << ">" << std::endl;
     indent++;


     ss << INDENT << "<params";
     if      ((c.Source == "A") and (c.Modulation == 10)) ss << " delsys=" << '"' << SYS_ATSC         << '"';
     else if ((c.Source == "A") and (c.Modulation == 10)) ss << " delsys=" << '"' << SYS_ATSC         << '"';
     else if ( c.Source == "A")                           ss << " delsys=" << '"' << SYS_DVBC_ANNEX_B << '"';
     else if ( c.Source == "C")                           ss << " delsys=" << '"' << SYS_DVBC_ANNEX_A << '"';
     else if ((c.Source == "T") and (c.DelSys == 0))      ss << " delsys=" << '"' << SYS_DVBT         << '"';
     else if ((c.Source == "T") and (c.DelSys == 1))      ss << " delsys=" << '"' << SYS_DVBT2        << '"';
     else if ((c.Source[0] == 'S') and (c.DelSys == 0))   ss << " delsys=" << '"' << SYS_DVBS         << '"';
     else if ((c.Source[0] == 'S') and (c.DelSys == 1))   ss << " delsys=" << '"' << SYS_DVBS2        << '"';

     if (c.Source[0] == 'S') ss << " center_frequency=" << '"' << as_double(c.Frequency)        << '"';
     else                    ss << " center_frequency=" << '"' << as_double(c.Frequency/1000.0) << '"';
     ss << ">" << std::endl;
     indent++;

     if (c.Source == "A") {
        if (c.Modulation == 10)         ss << INDENT << "<param modulation="   << '"' << "MODULATION = VSB_8"     << '"' << "/>" << std::endl;
        else if (c.Modulation == 11)    ss << INDENT << "<param modulation="   << '"' << "MODULATION = VSB_16"    << '"' << "/>" << std::endl;
        else if (c.Modulation == 256)   ss << INDENT << "<param modulation="   << '"' << "MODULATION = QAM_256"   << '"' << "/>" << std::endl;
        else                            ss << INDENT << "<param modulation="   << '"' << "MODULATION = QAM_AUTO"  << '"' << "/>" << std::endl;
        }
     else if (c.Source == "C") {
        ss << INDENT << "<param symbolrate="   << '"' << as_double(c.Symbolrate/1000) << '"' << "/>" << std::endl;
        ss << INDENT << "<param coderate="     << '"' << "FEC_NONE"                   << '"' << "/>" << std::endl;

        if (c.Modulation == 32)         ss << INDENT << "<param modulation="   << '"' << "MODULATION = QAM_32"    << '"' << "/>" << std::endl;
        else if (c.Modulation == 64)    ss << INDENT << "<param modulation="   << '"' << "MODULATION = QAM_64"    << '"' << "/>" << std::endl;
        else if (c.Modulation == 128)   ss << INDENT << "<param modulation="   << '"' << "MODULATION = QAM_128"   << '"' << "/>" << std::endl;
        else if (c.Modulation == 256)   ss << INDENT << "<param modulation="   << '"' << "MODULATION = QAM_256"   << '"' << "/>" << std::endl;
        else                            ss << INDENT << "<param modulation="   << '"' << "MODULATION = QAM_AUTO"  << '"' << "/>" << std::endl;
        }
     else if (c.Source == "T") {
        if (c.Modulation == 2)          ss << INDENT << "<param modulation="   << '"' << "MODULATION = QPSK"      << '"' << "/>" << std::endl;
        else if (c.Modulation == 16)    ss << INDENT << "<param modulation="   << '"' << "MODULATION = QAM_16"    << '"' << "/>" << std::endl;
        else if (c.Modulation == 64)    ss << INDENT << "<param modulation="   << '"' << "MODULATION = QAM_64"    << '"' << "/>" << std::endl;
        else if (c.Modulation == 256)   ss << INDENT << "<param modulation="   << '"' << "MODULATION = QAM_256"   << '"' << "/>" << std::endl;
        else                            ss << INDENT << "<param modulation="   << '"' << "MODULATION = QAM_AUTO"  << '"' << "/>" << std::endl;

        double bMHz = c.Bandwidth; while(bMHz > 10.0) bMHz /= 1000.0;
        ss << INDENT << "<param bandwidth=" << '"' << as_double(bMHz) << '"' << "/>" << std::endl;

        if (c.FEC == 0)                 ss << INDENT << "<param coderate="     << '"' << "FEC_NONE"               << '"' << "/>" << std::endl;
        else if (c.FEC == 12)           ss << INDENT << "<param coderate="     << '"' << "FEC_1_2"                << '"' << "/>" << std::endl;
        else if (c.FEC == 23)           ss << INDENT << "<param coderate="     << '"' << "FEC_2_3"                << '"' << "/>" << std::endl;
        else if (c.FEC == 34)           ss << INDENT << "<param coderate="     << '"' << "FEC_3_4"                << '"' << "/>" << std::endl;
        else if (c.FEC == 35)           ss << INDENT << "<param coderate="     << '"' << "FEC_3_5"                << '"' << "/>" << std::endl;
        else if (c.FEC == 45)           ss << INDENT << "<param coderate="     << '"' << "FEC_4_5"                << '"' << "/>" << std::endl;
        else if (c.FEC == 56)           ss << INDENT << "<param coderate="     << '"' << "FEC_5_6"                << '"' << "/>" << std::endl;
        else if (c.FEC == 67)           ss << INDENT << "<param coderate="     << '"' << "FEC_6_7"                << '"' << "/>" << std::endl;
        else if (c.FEC == 78)           ss << INDENT << "<param coderate="     << '"' << "FEC_7_8"                << '"' << "/>" << std::endl;
        else if (c.FEC == 89)           ss << INDENT << "<param coderate="     << '"' << "FEC_8_9"                << '"' << "/>" << std::endl;
        else if (c.FEC == 910)          ss << INDENT << "<param coderate="     << '"' << "FEC_9_10"               << '"' << "/>" << std::endl;
        else                            ss << INDENT << "<param coderate="     << '"' << "FEC_AUTO"               << '"' << "/>" << std::endl;

        if (c.Transmission == 1)        ss << INDENT << "<param transmission=" << '"' << "TRANSMISSION_MODE_1K"   << '"' << "/>" << std::endl;
        else if (c.Transmission == 2)   ss << INDENT << "<param transmission=" << '"' << "TRANSMISSION_MODE_2K"   << '"' << "/>" << std::endl;
        else if (c.Transmission == 4)   ss << INDENT << "<param transmission=" << '"' << "TRANSMISSION_MODE_4K"   << '"' << "/>" << std::endl;
        else if (c.Transmission == 8)   ss << INDENT << "<param transmission=" << '"' << "TRANSMISSION_MODE_8K"   << '"' << "/>" << std::endl;
        else if (c.Transmission == 16)  ss << INDENT << "<param transmission=" << '"' << "TRANSMISSION_MODE_16K"  << '"' << "/>" << std::endl;
        else if (c.Transmission == 32)  ss << INDENT << "<param transmission=" << '"' << "TRANSMISSION_MODE_32K"  << '"' << "/>" << std::endl;
        else                            ss << INDENT << "<param transmission=" << '"' << "TRANSMISSION_MODE_AUTO" << '"' << "/>" << std::endl;

        if (c.Guard == 4)               ss << INDENT << "<param guard="        << '"' << "GUARD_INTERVAL_1_4"     << '"' << "/>" << std::endl;
        else if (c.Guard == 8)          ss << INDENT << "<param guard="        << '"' << "GUARD_INTERVAL_1_8"     << '"' << "/>" << std::endl;
        else if (c.Guard == 16)         ss << INDENT << "<param guard="        << '"' << "GUARD_INTERVAL_1_16"    << '"' << "/>" << std::endl;
        else if (c.Guard == 32)         ss << INDENT << "<param guard="        << '"' << "GUARD_INTERVAL_1_32"    << '"' << "/>" << std::endl;
        else if (c.Guard == 128)        ss << INDENT << "<param guard="        << '"' << "GUARD_INTERVAL_1_128"   << '"' << "/>" << std::endl;
        else if (c.Guard == 19128)      ss << INDENT << "<param guard="        << '"' << "GUARD_INTERVAL_19_128"  << '"' << "/>" << std::endl;
        else if (c.Guard == 19256)      ss << INDENT << "<param guard="        << '"' << "GUARD_INTERVAL_19_256"  << '"' << "/>" << std::endl;
        else                            ss << INDENT << "<param guard="        << '"' << "GUARD_INTERVAL_AUTO"    << '"' << "/>" << std::endl;

        if ((c.Hierarchy > 0) and (c.Hierarchy < 999)) {
           if (c.Hierarchy == 1)        ss << INDENT << "<param hierarchy="    << '"' << "HIERARCHY_1"            << '"' << "/>" << std::endl;
           else if (c.Hierarchy == 2)   ss << INDENT << "<param hierarchy="    << '"' << "HIERARCHY_2"            << '"' << "/>" << std::endl;
           else if (c.Hierarchy == 4)   ss << INDENT << "<param hierarchy="    << '"' << "HIERARCHY_4"            << '"' << "/>" << std::endl;
           else                         ss << INDENT << "<param hierarchy="    << '"' << "HIERARCHY_AUTO"         << '"' << "/>" << std::endl;

           if (c.FEC_low == 0)          ss << INDENT << "<param coderate_LP="  << '"' << "FEC_NONE"               << '"' << "/>" << std::endl;
           else if (c.FEC_low == 12)    ss << INDENT << "<param coderate_LP="  << '"' << "FEC_1_2"                << '"' << "/>" << std::endl;
           else if (c.FEC_low == 23)    ss << INDENT << "<param coderate_LP="  << '"' << "FEC_2_3"                << '"' << "/>" << std::endl;
           else if (c.FEC_low == 34)    ss << INDENT << "<param coderate_LP="  << '"' << "FEC_3_4"                << '"' << "/>" << std::endl;
           else if (c.FEC_low == 35)    ss << INDENT << "<param coderate_LP="  << '"' << "FEC_3_5"                << '"' << "/>" << std::endl;
           else if (c.FEC_low == 45)    ss << INDENT << "<param coderate_LP="  << '"' << "FEC_4_5"                << '"' << "/>" << std::endl;
           else if (c.FEC_low == 56)    ss << INDENT << "<param coderate_LP="  << '"' << "FEC_5_6"                << '"' << "/>" << std::endl;
           else if (c.FEC_low == 67)    ss << INDENT << "<param coderate_LP="  << '"' << "FEC_6_7"                << '"' << "/>" << std::endl;
           else if (c.FEC_low == 78)    ss << INDENT << "<param coderate_LP="  << '"' << "FEC_7_8"                << '"' << "/>" << std::endl;
           else if (c.FEC_low == 89)    ss << INDENT << "<param coderate_LP="  << '"' << "FEC_8_9"                << '"' << "/>" << std::endl;
           else if (c.FEC_low == 910)   ss << INDENT << "<param coderate_LP="  << '"' << "FEC_9_10"               << '"' << "/>" << std::endl;
           else                         ss << INDENT << "<param coderate_LP="  << '"' << "FEC_AUTO"               << '"' << "/>" << std::endl;
           }
        if (c.DelSys == 1)
           if (c.StreamId  != 999)      ss << INDENT << "<param plp_id="       << '"' << c.StreamId               << '"' << "/>" << std::endl;
        }
     else if (c.Source[0] == 'S') {
        if (c.Polarization == 'H')      ss << INDENT << "<param polarization=" << '"' << "HORIZONTAL"             << '"' << "/>" << std::endl;
        else if (c.Polarization == 'V') ss << INDENT << "<param polarization=" << '"' << "VERTICAL"               << '"' << "/>" << std::endl;
        else if (c.Polarization == 'L') ss << INDENT << "<param polarization=" << '"' << "CIRCULAR_LEFT"          << '"' << "/>" << std::endl;
        else if (c.Polarization == 'R') ss << INDENT << "<param polarization=" << '"' << "CIRCULAR_RIGHT"         << '"' << "/>" << std::endl;

        ss << INDENT << "<param symbolrate="   << '"' << as_double(c.Symbolrate/1000)                             << '"' << "/>" << std::endl;

        if (c.FEC == 0)                 ss << INDENT << "<param coderate="     << '"' << "FEC_NONE"               << '"' << "/>" << std::endl;
        else if (c.FEC == 12)           ss << INDENT << "<param coderate="     << '"' << "FEC_1_2"                << '"' << "/>" << std::endl;
        else if (c.FEC == 23)           ss << INDENT << "<param coderate="     << '"' << "FEC_2_3"                << '"' << "/>" << std::endl;
        else if (c.FEC == 34)           ss << INDENT << "<param coderate="     << '"' << "FEC_3_4"                << '"' << "/>" << std::endl;
        else if (c.FEC == 35)           ss << INDENT << "<param coderate="     << '"' << "FEC_3_5"                << '"' << "/>" << std::endl;
        else if (c.FEC == 45)           ss << INDENT << "<param coderate="     << '"' << "FEC_4_5"                << '"' << "/>" << std::endl;
        else if (c.FEC == 56)           ss << INDENT << "<param coderate="     << '"' << "FEC_5_6"                << '"' << "/>" << std::endl;
        else if (c.FEC == 67)           ss << INDENT << "<param coderate="     << '"' << "FEC_6_7"                << '"' << "/>" << std::endl;
        else if (c.FEC == 78)           ss << INDENT << "<param coderate="     << '"' << "FEC_7_8"                << '"' << "/>" << std::endl;
        else if (c.FEC == 89)           ss << INDENT << "<param coderate="     << '"' << "FEC_8_9"                << '"' << "/>" << std::endl;
        else if (c.FEC == 910)          ss << INDENT << "<param coderate="     << '"' << "FEC_9_10"               << '"' << "/>" << std::endl;
        else                            ss << INDENT << "<param coderate="     << '"' << "FEC_AUTO"               << '"' << "/>" << std::endl;

        if (c.DelSys == 1) {
           if (c.Modulation == 2)       ss << INDENT << "<param modulation="   << '"' << "QPSK"                   << '"' << "/>" << std::endl;
           else if (c.Modulation == 5)  ss << INDENT << "<param modulation="   << '"' << "PSK_8"                  << '"' << "/>" << std::endl;
           else if (c.Modulation == 6)  ss << INDENT << "<param modulation="   << '"' << "APSK_16"                << '"' << "/>" << std::endl;
           else if (c.Modulation == 7)  ss << INDENT << "<param modulation="   << '"' << "APSK_32"                << '"' << "/>" << std::endl;
           else                         ss << INDENT << "<param modulation="   << '"' << "QAM_AUTO"               << '"' << "/>" << std::endl;

	        if (c.Pilot == 0)            ss << INDENT << "<param pilot="        << '"' << "PILOT_ON"               << '"' << "/>" << std::endl;
	        else if (c.Pilot == 1)       ss << INDENT << "<param pilot="        << '"' << "PILOT_OFF"              << '"' << "/>" << std::endl;
           else                         ss << INDENT << "<param pilot="        << '"' << "PILOT_AUTO"             << '"' << "/>" << std::endl;

           if (c.Rolloff == 20)         ss << INDENT << "<param rolloff="      << '"' << "ROLLOFF_20"             << '"' << "/>" << std::endl;
           else if (c.Rolloff == 25)    ss << INDENT << "<param rolloff="      << '"' << "ROLLOFF_25"             << '"' << "/>" << std::endl;
           else if (c.Rolloff == 35)    ss << INDENT << "<param rolloff="      << '"' << "ROLLOFF_35"             << '"' << "/>" << std::endl;
           else                         ss << INDENT << "<param rolloff="      << '"' << "ROLLOFF_AUTO"           << '"' << "/>" << std::endl;

           if (c.StreamId  != 999)      ss << INDENT << "<param plp_id="       << '"' << c.StreamId               << '"' << "/>" << std::endl;
           }
        }
     indent--;

     ss << INDENT << "</params>"      << std::endl; indent--;
     ss << INDENT << "</transponder>" << std::endl; indent--;
     } /* for loop */

  ss << INDENT << "</transponders>"   << std::endl;


  ss << INDENT << "<services>" << std::endl; indent++;
  for(auto c:List) {
     if (c.Name.empty()) c.Name = "empty";

     ss << INDENT << "<service"
        << " ONID=" << '"' << c.ONID << '"'
        << " TSID=" << '"' << c.TID  << '"'
        << " SID="  << '"' << c.SID  << '"'
        << ">" << std::endl;  indent++;

     if (not c.Name.empty()) {
        std::string name = c.Name;
        XmlString(name);
        ss << INDENT << "<name char256=" << '"' << name << '"' << "/>" << std::endl;
        }
     if (not c.Provider.empty()) {
        std::string provider = c.Provider;
        XmlString(provider);
        ss << INDENT << "<provider char256=" << '"' << provider << '"' << "/>" << std::endl;
        }
     if (c.PCR != 0)
        ss << INDENT << "<pcr pid=" << '"' << c.PCR << '"' << "/>" << std::endl;

     ss << INDENT << "<streams>" << std::endl; indent++;
     if (c.VPID.PID != 0) {
        ss << INDENT << "<stream"
           << " type=" << '"' << c.VPID.Type << '"'
           << " pid="  << '"' << c.VPID.PID  << '"';

        if (c.VPID.Type == 1)       ss << " description=" << '"' << "MPEG-1 Video stream, ISO/IEC 11172"                    << '"';
        else if (c.VPID.Type == 2)  ss << " description=" << '"' << "MPEG-2 Part 1 Video stream, ISO/IEC 13818-1"           << '"';
        else if (c.VPID.Type == 16) ss << " description=" << '"' << "ISO/IEC 14496-2 Visual"                                << '"';
        else if (c.VPID.Type == 27) ss << " description=" << '"' << "AVC Video stream, ITU-T Rec. H.264 | ISO/IEC 14496-10" << '"';
        else if (c.VPID.Type == 36) ss << " description=" << '"' << "HEVC Video stream, ITU-T Rec. H.265 | ISO/IEC 23008-1" << '"';

        ss << "/>" << std::endl;
        }

     for(int i = 0; i < c.APIDs.Count(); i++) {
        if (c.APIDs[i].PID == 0) continue;
        ss << INDENT << "<stream"
           << " type=" << '"' << c.APIDs[i].Type << '"'
           << " pid="  << '"' << c.APIDs[i].PID  << '"';
        if (c.APIDs[i].Type == 3)       ss << " description="   << '"' << "MPEG-1 Audio stream, ISO/IEC 11172"                  << '"';
        else if (c.APIDs[i].Type == 4)  ss << " description="   << '"' << "MPEG-2 Part 1 Audio stream, ISO/IEC 13818-3"         << '"';
        else if (c.APIDs[i].Type == 15) ss << " description="   << '"' << "ADTS Audio Stream (usually AAC)"                     << '"';
        else if (c.APIDs[i].Type == 17) ss << " description="   << '"' << "ISO/IEC 14496-3 Audio with LATM transport syntax"    << '"';
        else if (c.APIDs[i].Type == 28) ss << " description="   << '"' << "ISO/IEC 14496-3 Audio, w/o add transport syntax"     << '"';
        if (not c.APIDs[i].Lang.empty())ss << " language_code=" << '"' << c.APIDs[i].Lang                                       << '"';
        ss << "/>" << std::endl;
        }

     for(int i = 0; i < c.DPIDs.Count(); i++) {
        if (c.DPIDs[i].PID == 0) continue;
        ss << INDENT << "<stream"
           << " type=" << '"' << 6 << '"'
           << " pid="  << '"' << c.DPIDs[i].PID  << '"';
        if (c.DPIDs[i].Type == 106)      ss << " description="   << '"' << "AC3 Audio stream"                                   << '"';
        else if (c.DPIDs[i].Type == 122) ss << " description="   << '"' << "EAC3 Audio stream"                                  << '"';
        else if (c.DPIDs[i].Type == 123) ss << " description="   << '"' << "DTS Audio stream"                                   << '"';
        else if (c.DPIDs[i].Type == 124) ss << " description="   << '"' << "AAC Audio stream"                                   << '"';
        if (not c.DPIDs[i].Lang.empty()) ss << " language_code=" << '"' << c.DPIDs[i].Lang                                      << '"';
        ss << "/>" << std::endl;
        }

     if (c.TPID != 0) {
        ss << INDENT << "<stream"
           << " type=" << '"' << 6 << '"'
           << " pid="  << '"' << c.TPID << '"'
           << " description=" << '"' << "teletext" << '"'
           << "/>" << std::endl;
        }

     for(int i = 0; i < c.SPIDs.Count(); i++) {
        if (c.SPIDs[i].PID == 0) continue;
        ss << INDENT << "<stream"
           << " type=" << '"' << 6 << '"'
           << " pid="  << '"' << c.SPIDs[i].PID  << '"'
           << " description=" << '"' << "subtitling" << '"';
        if (not c.SPIDs[i].Lang.empty()) ss << " language_code=" << '"' << c.SPIDs[i].Lang << '"';
        ss << "/>" << std::endl;
        }    
     indent--; ss << INDENT << "</streams>" << std::endl;

     if (c.CAIDs.Count() > 0) {
        ss << INDENT << "<CA_systems>" << std::endl; indent++;
        for(int i = 0; i < c.CAIDs.Count(); i++) {
           if (c.CAIDs[i] == 0) continue;
           ss << INDENT << "<CA_system";
           if (not CaName(c.CAIDs[i]).empty()) ss << " name="  << '"' << CaName(c.CAIDs[i])  << '"';
           ss << " ca_id=" << '"' << c.CAIDs[i] << '"' << "/>" << std::endl;
           }
        indent--;  ss << INDENT << "</CA_systems>" << std::endl;
        }

     indent--;  ss << INDENT << "</service>" << std::endl;
     }
  indent--; ss << INDENT << "</services>" << std::endl;

  indent--; ss << INDENT << "</service_list>" << std::endl;

  ss << std::endl;
  OutputLine(ss.str());
}

std::string CaName(int id) {
//int minor = id & 0xFF;
  int major = id >> 8;

  if      (major == 0x01) return "Seca Mediaguard";
  else if (major == 0x05) return "Viaccess";
  else if (major == 0x06) return "Irdeto";
  else if (major == 0x07) return "DigiCipher-2";
  else if (major == 0x09) return "NDS/Cisco Videoguard";
  else if (major == 0x0B) return "Conax";
  else if (major == 0x0D) return "Cryptoworks";
  else if (major == 0x0E) return "PowerVu";
  else if (major == 0x10) return "RAS (Remote Authorization System)";
  else if (major == 0x17) return "Betaresearch (or Tunnel)";
  else if (major == 0x18) return "Nagravision";
  else if (major == 0x22) return "Codicrypt";
  else if (major == 0x26) return "BISS";
  else if (id == 0x0464)  return "EuroDec";
  else if (id == 0x2719)  return "InCrypt Cas";
  else if (id == 0x4347)  return "Crypton";
  else if (id == 0x4800)  return "Accessgate";
  else if (id == 0x4900)  return "China Crypt";
  else if (id == 0x4A02)  return "Tongfang";
  else if (id == 0x4A10)  return "EasyCas";
  else if (id == 0x4A20)  return "AlphaCrypt";
  else if (id == 0x4A60)  return "Skycrypt / Neotioncrypt / Neotion SHL";
  else if (id == 0x4A61)  return "Skycrypt / Neotioncrypt / Neotion SHL";
  else if (id == 0x4A63)  return "Skycrypt / Neotioncrypt / Neotion SHL";
  else if (id == 0x4A70)  return "Dreamcrypt";
  else if (id == 0x4A80)  return "ThalesCrypt";
  else if (id == 0x4AA1)  return "KeyFly";
  else if (id == 0x4ABF)  return "CTI-CAS";
  else if (id == 0x4AD0)  return "X-Crypt";
  else if (id == 0x4AD1)  return "X-Crypt";
  else if (id == 0x4AD4)  return "OmniCrypt";
  else if (id == 0x4AE0)  return "DRE-Crypt";
  else if (id == 0x4AE1)  return "DRE-Crypt";
  else if (id == 0x4AE4)  return "CoreCrypt";
  else if (id == 0x4AEA)  return "Cryptoguard";
  else if (id == 0x4AEB)  return "Abel Quintic";
  else if (id == 0x4AF0)  return "ABV CAS";
  else if (id == 0x4AFC)  return "Panaccess";
  else if (id == 0x4B19)  return "RCAS oder RIDSYS cas";
  else if (id == 0x4B30)  return "Vicas";
  else if (id == 0x4B31)  return "Vicas";
  else if (id == 0x5448)  return "Gospell VisionCrypt";
  else if (id == 0x5501)  return "Greif";
  else if (id == 0x5581)  return "Bulcrypt";
  else if (id == 0x7BE1)  return "DRE-Crypt";
  else if (id == 0xA101)  return "RosCrypt-M";
  else if (id == 0xEAD0)  return "InCrypt Cas";
  return "";
}
