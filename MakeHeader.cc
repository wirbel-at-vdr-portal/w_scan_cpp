#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>

bool ReadFile(std::string aFileName, std::stringstream& Dest) {
  std::ifstream is(aFileName.c_str());
  if (is) {
     Dest << is.rdbuf();
     is.close();
     return true;
     }
  return false;
}

bool WriteFile(std::string aFileName, std::vector<std::string>& Src) {
  std::ofstream fs(aFileName.c_str());
  if (fs.fail())
     return false;
  for(auto s:Src)
     fs << s << std::endl;
  return true;
}

int main(int argc, char** args) {
  std::string s, fname(args[1]);
  std::stringstream ss;
  std::vector<std::string> cpp, hdr;
  bool start = false;

  fname += "satip.c";
  if (not ReadFile(fname, ss))
     return 1;

  while(std::getline(ss, s)) {
     if (s.back() == '\r') s.pop_back(); // DOS \r\n -> UNIX \n

     if (not start and s.find("GITVERSION") != std::string::npos) {
        start = true;
        cpp.push_back("#include \"satip.h\"");
        hdr.push_back("#pragma once\n\nclass cSatipDiscoverServers;\n");
        }
     if (not start)
        cpp.push_back(s);
     else {
        if ((s.find("       const char") == 0) or
            (s.find("static const char") == 0))
           s.erase(0, 7);
        hdr.push_back(s);
        }
     if (s.find("};") != std::string::npos and s.size() < 8)
        start = false;
     }

  WriteFile(fname, cpp);

  fname.back() = 'h';
  WriteFile(fname, hdr);

  return 0;
}
