#ifndef SUBPROCESS_ERROR_H
#define SUBPROCESS_ERROR_H

#include <exception>
#include <string>

class SubprocessError : public std::exception
{
private:
  std::string message;

public:
  SubprocessError(const std::string &msg) : message(msg) {}

  virtual const char *what() const noexcept override { return message.c_str(); }
};

class SubprocessUserAbort : public std::exception
{
private:
  std::string message;

public:
  SubprocessUserAbort(const std::string &msg) : message(msg) {}

  virtual const char *what() const noexcept override { return message.c_str(); }
};

#endif // SUBPROCESS_ERROR_H
