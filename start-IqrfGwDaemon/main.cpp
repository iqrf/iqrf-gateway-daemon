#include <Shaper.h>
#include <Trace.h>
#include <iostream>

TRC_INIT_MNAME("IqrfGwDaemon-start");

int main(int argc, char** argv)
{
  std::cout << "startup ... " << std::endl;
  shapeInit(argc, argv);
  shapeRun();
  return 0;
}
