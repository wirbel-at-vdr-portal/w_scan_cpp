/******************************************************************************
 * w_scan_cpp - a dtv channel scanner based on VDR (www.tvdr.de) and it's
 * Plugins.
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <iostream>
#include <iomanip>

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

std::vector<std::string> split(const std::string& s, const char delim);
std::string LowerCase(std::string s);
std::string UpperCase(std::string s);
std::string FrontFill(std::string s, size_t n);
std::string BackFill(std::string s, size_t n);
std::string IntToStr(int64_t n, size_t Zeros = 0);
std::string IntToHex(uint64_t n, size_t Digits = 0);
std::string FloatToStr(double d, int Precisision = 1);
std::string ExpToStr(double d);
std::string VdrSource(std::string s);
void Sleep(size_t msec);
bool InitCharTables(void);
std::vector<std::string> ReadFile(std::string aFileName);

class cFileList {
private:
  std::vector<std::string> l;
public:
  cFileList(std::string aDirectory, std::string Filter = "") {
     for(DIR* dirp = opendir(aDirectory.c_str()); dirp; ) {
        errno = 0;
        struct dirent* d = readdir(dirp);

        if (d != nullptr) {
           std::string name(d->d_name);
           if (name.find(Filter) != std::string::npos)
              l.push_back(name);
           }
        else {
           closedir(dirp);
           dirp = nullptr;
           }
        }
     }
  size_t size(void) { return l.size(); }
  std::vector<std::string> List(void) { return l; }
};

class File {
public:
  static bool Exists(std::string FileName) {
    return (access(FileName.c_str(), R_OK) == 0);
    }
};
