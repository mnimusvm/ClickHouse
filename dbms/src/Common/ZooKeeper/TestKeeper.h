#pragma once

#include <mutex>
#include <map>
#include <atomic>
#include <thread>
#include <chrono>

#include <Poco/Timespan.h>
#include <Common/ZooKeeper/IKeeper.h>
#include <Common/ThreadPool.h>
#include <Common/ConcurrentBoundedQueue.h>


namespace Coordination
{

struct TestKeeperRequest;
using TestKeeperRequestPtr = std::shared_ptr<TestKeeperRequest>;


class TestKeeper : public IKeeper
{
public:
    TestKeeper(const String & root_path, Poco::Timespan operation_timeout);
    ~TestKeeper() override;

    bool isExpired() const override { return expired; }
    int64_t getSessionID() const override { return 0; }


    void create(
            const String & path,
            const String & data,
            bool is_ephemeral,
            bool is_sequential,
            const ACLs & acls,
            CreateCallback callback) override;

    void remove(
            const String & path,
            int32_t version,
            RemoveCallback callback) override;

    void exists(
            const String & path,
            ExistsCallback callback,
            WatchCallback watch) override;

    void get(
            const String & path,
            GetCallback callback,
            WatchCallback watch) override;

    void set(
            const String & path,
            const String & data,
            int32_t version,
            SetCallback callback) override;

    void list(
            const String & path,
            ListCallback callback,
            WatchCallback watch) override;

    void check(
            const String & path,
            int32_t version,
            CheckCallback callback) override;

    void multi(
            const Requests & requests,
            MultiCallback callback) override;


    struct Node
    {
        String data;
        ACLs acls;
        bool is_ephemeral = false;
        bool is_sequental = false;
        Stat stat{};
        int32_t seq_num = 0;
    };

    using Container = std::map<std::string, Node>;

    using WatchCallbacks = std::vector<WatchCallback>;
    using Watches = std::map<String /* path, relative of root_path */, WatchCallbacks>;

private:
    using clock = std::chrono::steady_clock;

    struct RequestInfo
    {
        TestKeeperRequestPtr request;
        ResponseCallback callback;
        WatchCallback watch;
        clock::time_point time;
    };

    Container container;

    String root_path;
    ACLs default_acls;

    Poco::Timespan operation_timeout;

    std::mutex push_request_mutex;
    std::atomic<bool> expired{false};

    int64_t zxid = 0;

    Watches watches;
    Watches list_watches;   /// Watches for 'list' request (watches on children).

    void createWatchCallBack(const String & path);

    using RequestsQueue = ConcurrentBoundedQueue<RequestInfo>;
    RequestsQueue requests_queue{1};

    void pushRequest(RequestInfo && request);

    void finalize();

    ThreadFromGlobalPool processing_thread;

    void processingThread();
};

}
