#include <string>
#include  "TaskQueue.h"

#include <gtest/gtest.h>

// global for results
std::string result, result_delayed;

void handleQueue(std::string a) {
    result += a;
}

void handleQueueDelayed(std::string a) {
    result_delayed += a;
    sleep(1);
}

/* test size reporting */
TEST(TaskQueue, Size) {
    auto tq = TaskQueue<std::string>(handleQueue);
    tq.pushToQueue("8");
    tq.pushToQueue("9");

    EXPECT_EQ(tq.size(), 2);
    // give it some time
    sleep(1);

    EXPECT_EQ(result, std::string("89"));
    // clear for next test
    result = "";
}

/* Same as above but check size using push method */
TEST(TaskQueue, SizeFromPush) {
    auto tq = TaskQueue<std::string>(handleQueue);
    tq.pushToQueue("8");
    
    EXPECT_EQ(tq.pushToQueue("9"), 2);
    sleep(1);
}

/* Added some delay for task processing */
TEST(TaskQueue, TimeConsumingTask) {
    auto tq = TaskQueue<std::string>(handleQueueDelayed);
    tq.pushToQueue("8");
    
    sleep(2);
    EXPECT_EQ(tq.pushToQueue("9"), 1);
    sleep(2);
    EXPECT_EQ(tq.size(), 0);

    EXPECT_EQ(result_delayed, std::string("89"));
}

/* Test queue stop 
* ! job remains in fifo just queue is stopped
*/
TEST(TaskQueue, StopQueue) {
    auto tq = TaskQueue<std::string>(handleQueueDelayed);
    tq.pushToQueue("8");
    tq.stopQueue();
    
    EXPECT_EQ(tq.size(), 1);
}
