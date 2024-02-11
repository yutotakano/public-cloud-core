#ifndef TRACE_H
#define TRACE_H

#include "quill/Quill.h"

#define TRACE_FUNCTION_ENTER(logger)                                           \
  LOG_TRACE_L3(logger, "Entering {}", __PRETTY_FUNCTION__)

#define TRACE_FUNCTION_EXIT(logger)                                            \
  LOG_TRACE_L3(logger, "Exiting {}", __PRETTY_FUNCTION__)

#define TRACE(...) LOG_TRACE_L3(__VA_ARGS__)

#endif // TRACE_H
