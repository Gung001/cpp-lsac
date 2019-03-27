#pragma once

#include "Log.h"
#include <boost/program_options/options_description.hpp>

namespace dev
{
boost::program_options::options_description createLoggingProgramOptions(
    unsigned _lineLength, LoggingOptions& _options);

}  // namespace dev