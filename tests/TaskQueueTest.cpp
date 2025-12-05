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

#include <gtest/gtest.h>
#include "TaskQueue.h"

#include <chrono>
#include <thread>

namespace task_queue_test {

class TaskQueueTest : public ::testing::Test {
protected:
  int result;

  void multHandler(int val) {
    result = val * val;
  }

  void multHandlerDelay(int val) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    result = val * val;
  }

  void setUp() {
    result = 0;
  }
};

TEST_F(TaskQueueTest, SizeTest) {
  TaskQueue<int> queue([this](int val) {
    multHandler(val);
  });
  EXPECT_EQ(queue.size(), 0);

  queue.pushToQueue(2);
  queue.pushToQueue(3);
  EXPECT_EQ(queue.size(), 2);

  queue.startQueue();
  // let queue process tasks
  std::this_thread::sleep_for(std::chrono::seconds(1));

  EXPECT_EQ(queue.size(), 0);
  EXPECT_EQ(result, 9);
}

TEST_F(TaskQueueTest, PushToQueueTest) {
  TaskQueue<int> queue([this](int val) {
    multHandler(val);
  });
  EXPECT_EQ(queue.size(), 0);
  EXPECT_EQ(queue.pushToQueue(2), 1);
}

TEST_F(TaskQueueTest, WorkerTest) {
  TaskQueue<int> queue([this](int val) {
    multHandlerDelay(val);
  });
  EXPECT_EQ(queue.size(), 0);

  queue.pushToQueue(2);
  queue.pushToQueue(3);

  std::this_thread::sleep_for(std::chrono::milliseconds(1100));
  EXPECT_EQ(result, 4);

  std::this_thread::sleep_for(std::chrono::milliseconds(1100));
  EXPECT_EQ(result, 9);

  EXPECT_EQ(queue.size(), 0);
}

TEST_F(TaskQueueTest, StartQueueTest) {
  TaskQueue<int> queue([this](int val) {
    multHandlerDelay(val);
  });
  EXPECT_EQ(queue.size(), 0);

  queue.pushToQueue(2);
  queue.stopQueue();

  std::this_thread::sleep_for(std::chrono::milliseconds(1100));
  EXPECT_EQ(queue.size(), 1);

  queue.startQueue();
  std::this_thread::sleep_for(std::chrono::milliseconds(1100));

  EXPECT_EQ(queue.size(), 0);
}

TEST_F(TaskQueueTest, StopQueueTest) {
  TaskQueue<int> queue([this](int val) {
    multHandlerDelay(val);
  });
  EXPECT_EQ(queue.size(), 0);

  queue.pushToQueue(2);
  queue.stopQueue();

  EXPECT_EQ(queue.size(), 1);

  queue.pushToQueue(2);
  EXPECT_EQ(queue.size(), 2);
  EXPECT_EQ(result, 0);
}

TEST_F(TaskQueueTest, ClearQueueTest) {
  TaskQueue<int> queue([this](int val) {
    multHandler(val);
  });
  EXPECT_EQ(queue.size(), 0);

  queue.stopQueue();

  queue.pushToQueue(2);
  queue.pushToQueue(2);
  queue.pushToQueue(2);

  EXPECT_EQ(queue.size(), 3);

  queue.clearQueue();

  EXPECT_EQ(queue.size(), 0);
}

}
