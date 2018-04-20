#include "VersionInfo.h"
#include <Shaper.h>
#include <StaticComponentMap.h>
#include <Trace.h>
#include <iostream>

TRC_INIT_MNAME("IqrfGwDaemon-start");

extern "C" {
  const shape::ComponentMeta& get_component_shape__LauncherService(unsigned long* compiler, unsigned long* typehash);
}

void staticInit()
{
  void* component = nullptr;
  unsigned long compiler = 0;
  unsigned long hashcode = 0;
  unsigned long expectedCompiler = (unsigned long)SHAPE_PREDEF_COMPILER;
  unsigned long expectedHashcode = std::type_index(typeid(shape::ComponentMeta)).hash_code();

  shape::ComponentMap::get().setComponent(&get_component_shape__LauncherService(&compiler, &hashcode));
}

int main(int argc, char** argv)
{
  std::cout <<
    "============================================================================" << std::endl <<
    PAR(DAEMON_VERSION) << PAR(BUILD_TIMESTAMP) << std::endl << std::endl <<
    "Copyright 2015 - 2017 MICRORISC s.r.o." << std::endl <<
    "Copyright 2018 IQRF Tech s.r.o." << std::endl <<
    "============================================================================" << std::endl;

  std::cout << "startup ... " << std::endl;
  staticInit();
  shapeInit(argc, argv);
  shapeRun();
  return 0;
}
