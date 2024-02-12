#ifndef UTILS_H
#define UTILS_H

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

  // trim from start (in place)
  static inline const char *ws = " \t\n\r\f\v";

  // trim from end of string (right)
  static inline std::string &rtrim(std::string &s, const char *t = ws)
  {
    s.erase(s.find_last_not_of(t) + 1);
    return s;
  }

  // trim from beginning of string (left)
  static inline std::string &ltrim(std::string &s, const char *t = ws)
  {
    s.erase(0, s.find_first_not_of(t));
    return s;
  }

  // trim from both ends of string (right then left)
  static inline std::string &trim(std::string &s, const char *t = ws)
  {
    return ltrim(rtrim(s, t), t);
  }
};

#endif // UTILS_H
