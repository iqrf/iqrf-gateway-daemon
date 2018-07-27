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


extern "C" {
  const shape::ComponentMeta& get_component_shape__LauncherService(unsigned long* compiler, unsigned long* typehash);
  const shape::ComponentMeta& get_component_shape__ConfigurationService(unsigned long* compiler, unsigned long* typehash);
}

void staticInit()
{
  void* component = nullptr;
  unsigned long compiler = 0;
  unsigned long hashcode = 0;
  unsigned long expectedCompiler = (unsigned long)SHAPE_PREDEF_COMPILER;
  unsigned long expectedHashcode = std::type_index(typeid(shape::ComponentMeta)).hash_code();

  shape::ComponentMap::get().setComponent(&get_component_shape__LauncherService(&compiler, &hashcode));
  shape::ComponentMap::get().setComponent(&get_component_shape__ConfigurationService(&compiler, &hashcode));
}

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
  staticInit();
  shapeInit(argc, argv);
  shapeRun();
  return 0;
}
