#pragma once

#include "ISchedulerService.h"
#include "ShapeProperties.h"
#include "ITraceService.h"
#include <string>

namespace iqrfgw {
  class Scheduler : public ISchedulerService
  {
  public:
    Scheduler();
    virtual ~Scheduler();

    void activate(const shape::Properties *props = 0);
    void deactivate();
    void modify(const shape::Properties *props);

    void attachInterface(shape::ITraceService* iface);
    void detachInterface(shape::ITraceService* iface);

    void start() override;
    void stop() override;

    void registerMessageHandler(const std::string& clientId, TaskHandlerFunc fun) override;
    void unregisterMessageHandler(const std::string& clientId) override;
    std::vector<std::string> getMyTasks(const std::string& clientId) const override;
    std::string getMyTask(const std::string& clientId, const TaskHandle& hndl) const override;
    TaskHandle scheduleTaskAt(const std::string& clientId, const std::string& task, const std::chrono::system_clock::time_point& tp) override;
    TaskHandle scheduleTaskPeriodic(const std::string& clientId, const std::string& task, const std::chrono::seconds& sec,
      const std::chrono::system_clock::time_point& tp) override;
    void removeAllMyTasks(const std::string& clientId) override;
    void removeTask(const std::string& clientId, TaskHandle hndl) override;
    void removeTasks(const std::string& clientId, std::vector<TaskHandle> hndls) override;

  private:
  };
}
