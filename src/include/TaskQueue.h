/**
 * Copyright 2015-2025 IQRF Tech s.r.o.
 * Copyright 2019-2025 MICRORISC s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "TimeConversion.h"

#include <functional>
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <queue>
#include <iostream>
#include <iomanip>

/// \class TaskQueue
/// \brief Maintain queue of tasks and invoke sequential processing
/// \details
/// Provide asynchronous processing of incoming tasks of type T in dedicated worker thread.
/// The tasks are processed in FIFO way. Processing function is passed as parameter in constructor.
template <class T>
class TaskQueue {
public:
  /// Processing function type
  typedef std::function<void(T)> ProcessTaskFunc;

  /// \brief constructor
  /// \param [in] processTaskFunc processing function
  /// \details
  /// Processing function is used in dedicated worker thread to process incoming queued tasks.
  /// The function must be thread safe. The worker thread is started.
  TaskQueue(ProcessTaskFunc processTaskFunc): m_processTaskFunc(processTaskFunc) {
    m_taskPushed = false;
    m_runWorkerThread = true;
    m_workerThread = std::thread(&TaskQueue::worker, this);
  }

  /// \brief destructor
  /// \details
  /// Stops working thread
  virtual ~TaskQueue() {
    stopQueue();
  }

  /// \brief Push task to queue
  /// \param [in] task object to push to queue
  /// \return size of queue
  /// \details
  /// Pushes task to queue to be processed in worker thread. The task type T has to be copyable
  /// as the copy is pushed to queue container
  size_t pushToQueue(const T& task) {
    size_t retval = 0;
    {
      std::unique_lock<std::mutex> lock(m_taskQueueMutex);
      m_taskQueue.push(task);
      retval = m_taskQueue.size();
      m_taskPushed = true;
    }
    m_conditionVariable.notify_all();
    return retval;
  }

  /// @brief Start queue
  /// \details
  /// Starts worker thread
  void startQueue() {
    if (m_runWorkerThread) {
      return;
    }

    m_taskPushed = size() > 0;
    m_runWorkerThread = true;
    m_workerThread = std::thread(&TaskQueue::worker, this);

    if (m_taskPushed) {
      m_conditionVariable.notify_all();
    }
  }

  /// \brief Stop queue
  /// \details
  /// Stops worker thread
  void stopQueue() {
    {
      std::unique_lock<std::mutex> lock(m_taskQueueMutex);
      m_runWorkerThread = false;
      m_taskPushed = true;
    }
    m_conditionVariable.notify_all();

    if (m_workerThread.joinable()) {
      m_workerThread.join();
    }
  }

  /// \brief Clear queue
  /// \details
  /// Clears queue tasks
  void clearQueue() {
    {
      std::unique_lock<std::mutex> lock(m_taskQueueMutex);
      std::queue<T> empty;
      std::swap(m_taskQueue, empty);
    }
  }

  /// \brief Get actual queue size
  /// \return queue size
  size_t size() {
    size_t retval = 0;
    {
      std::unique_lock<std::mutex> lock(m_taskQueueMutex);
      retval = m_taskQueue.size();
    }
    return retval;
  }

  /// \brief Get queue active state
  /// \return Queue active state
  bool isActive() {
    bool active = false;
    {
      std::unique_lock<std::mutex> lock(m_taskQueueMutex);
      active = m_runWorkerThread;
    }
    return active;
  }

private:
  /// Worker thread function
  void worker() {
    std::unique_lock<std::mutex> lock(m_taskQueueMutex, std::defer_lock);

    while (m_runWorkerThread) {

      //wait for something in the queue
      lock.lock();
      m_conditionVariable.wait(lock, [&] { return m_taskPushed; }); //lock is released in wait
      //lock is reacquired here
      m_taskPushed = false;

      while (m_runWorkerThread) {
        if (!m_taskQueue.empty()) {
          auto task = m_taskQueue.front();
          m_taskQueue.pop();
          lock.unlock();
          m_processTaskFunc(task);
        } else {
          lock.unlock();
          break;
        }
        lock.lock(); //lock for next iteration
      }
    }
  }

  /// Mutex
  std::mutex m_taskQueueMutex;
  /// Condition variable
  std::condition_variable m_conditionVariable;
  /// Task queue
  std::queue<T> m_taskQueue;
  /// Task pushed to queue, ready to be handled
  bool m_taskPushed;
  /// Run worker thread
  bool m_runWorkerThread;
  /// Worker thread
  std::thread m_workerThread;
  /// Task function
  ProcessTaskFunc m_processTaskFunc;
};

