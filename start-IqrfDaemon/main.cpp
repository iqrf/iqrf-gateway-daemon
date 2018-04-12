#include "VersionInfo.h"
#include <Shaper.h>
#include <Trace.h>
#include <iostream>

TRC_INIT_MNAME("IqrfGwDaemon-start");

int main(int argc, char** argv)
{
  std::cout <<
    "============================================================================" << std::endl <<
    PAR(DAEMON_VERSION) << PAR(BUILD_TIMESTAMP) << std::endl << std::endl <<
    "Copyright 2015 - 2017 MICRORISC s.r.o." << std::endl <<
    "Copyright 2018 IQRF Tech s.r.o." << std::endl <<
    "============================================================================" << std::endl;

  std::cout << "startup ... " << std::endl;
  shapeInit(argc, argv);
  shapeRun();
  return 0;
}
