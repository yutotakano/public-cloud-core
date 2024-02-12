#ifndef UTILS_H
#define UTILS_H

#include "trace.h"
#include <sstream>
#include <string>
#include <vector>

class Utils
{
public:
  static std::vector<std::string> split(const std::string &s, char delim)
  {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    while (getline(ss, item, delim))
    {
      result.push_back(item);
    }

    return result;
  }
};

#endif // UTILS_H
