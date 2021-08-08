/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <algorithm>
#include <langinfo.h>
#include <locale.h>
#include "Helpers.h"

std::vector<std::string> split(const std::string& s, const char delim) {
  std::stringstream ss(s);
  std::vector<std::string> result;
  std::string item;
  while(std::getline(ss, item, delim))
     result.push_back(item);
  return result;
}

std::string LowerCase(std::string s) {
  std::string str(s);
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
  return str;
}

std::string UpperCase(std::string s) {
  std::string str(s);
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
  return str;
}

std::string FrontFill(std::string s, size_t n) {
  std::string result(s);
  while(result.size() < n) result.insert(0, 1, ' ');
  return result;
}

std::string BackFill(std::string s, size_t n) {
  std::string result(s);
  while(result.size() < n) result.append(1, ' ');
  return result;
}

template<class T> std::string ToString(std::string format, T aVar) {
  char buf[256];
  snprintf(buf, 256-1, format.c_str(), aVar);
  return buf;
};

std::string IntToStr(int64_t n, size_t Zeros) {
  return ToString("%0" + std::to_string(Zeros) + "lld", (long long) n);
}

std::string IntToHex(uint64_t n, size_t Digits) {
  return ToString("0x%.0" + std::to_string(Digits) + "llX", (long long) n);
}

std::string FloatToStr(double d, int Precision) {
  return ToString("%." + std::to_string(Precision) + "f", d);
}

std::string ExpToStr(double d) {
  return ToString("%.3e", d);
}

std::string VdrSource(std::string s) {
  size_t p  = s.find_first_of("EWew");
  size_t p2 = s.find(".");

  if (p == std::string::npos)
     return s;

  char c = s[p];
  if (c == 'e') c = 'E';
  if (c == 'w') c = 'W';

  if (p2 != std::string::npos) {
     // VDR type.
     if (s[p2+1] == '0') // S13.0E -> S13E
        return s.substr(0,p2) + c;
     return s;
     }
  if ((p+1) == s.size()) // S33E
     return s;

  // w_scan type.
  if (s[p+1] == '0')
     return s.substr(0,p) + c;

  s[p] = '.';
  return s + c;
}

void Sleep(size_t msec) {
  std::this_thread::sleep_for(std::chrono::milliseconds(msec));
}

bool InitCharTables(void) {
  char* CodeSet;
  if (setlocale(LC_CTYPE, "") != nullptr)
     CodeSet = nl_langinfo(CODESET);
  else {
     char* LangEnv = getenv("LANG");
     if (LangEnv) {
        CodeSet = strchr(LangEnv, '.');
        if (CodeSet)
           CodeSet++;
        }
     else
        CodeSet = nullptr;
     }

  if (CodeSet) {
     bool result = SI::SetSystemCharacterTable(CodeSet);
     if (not result)
        ErrorMessage("SetSystemCharacterTables(" + std::string(CodeSet) + ") failed");
     cCharSetConv::SetSystemCharacterTable(CodeSet);
     return result;
     }
  else
     return false;
}

std::vector<std::string> ReadFile(std::string aFileName) {
  std::vector<std::string> result;
  std::ifstream is(aFileName.c_str());
  if (is) {
     std::stringstream ss;
     std::string s;
     size_t p;
     ss << is.rdbuf();
     is.close();
     while(std::getline(ss, s)) {
        if (s.empty() or s[0] == ':')
           continue;
        p = s.find_first_of(",;:");
        if (p != std::string::npos)
           s = s.substr(0,p);
        result.push_back(s);
        }
     }
  return result;
}
