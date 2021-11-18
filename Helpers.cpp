/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#include <string>
#include <langinfo.h>
#include <locale.h>
#include "Helpers.h"


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

