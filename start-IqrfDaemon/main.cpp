#include "VersionInfo.h"
#include <Shaper.h>
#include <StaticComponentMap.h>
#include <Trace.h>
#include <iostream>

TRC_INIT_MNAME("iqrfgd2");
#ifdef TRC_CHANNEL
#undef TRC_CHANNEL
#endif
//Shape buffer channel
#define TRC_CHANNEL 0


int main(int argc, char** argv)
{
  if (argc == 2 && argv[1] == std::string("version")) {
    std::cout << DAEMON_VERSION << " " << BUILD_TIMESTAMP << std::endl;
    return 0;
  }

  std::ostringstream header;
  header <<
      "============================================================================" << std::endl <<
      PAR(DAEMON_VERSION) << PAR(BUILD_TIMESTAMP) << std::endl << std::endl <<
      "Copyright 2015 - 2017 MICRORISC s.r.o." << std::endl <<
      "Copyright 2018 IQRF Tech s.r.o." << std::endl <<
      "============================================================================" << std::endl;

  std::cout << header.str();
  TRC_INFORMATION(header.str());

  std::cout << "startup ... " << std::endl;
  shapeInit(argc, argv);
  shapeRun();
  return 0;
}
