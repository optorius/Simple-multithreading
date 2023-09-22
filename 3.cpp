#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cassert>
#include <functional>

namespace custom
{

class barrier
{
public:
    using callback_fn = std::function< void() >;

public:
    barrier( uint16_t thread_count, callback_fn callback )
        : mutex_()
        , cv_()
        , thread_count_( thread_count )
        , count_ ( thread_count )
        , waiting_count_ ( 0 )
        , callback_( callback )

    {
        assert( thread_count > 0 );
        assert( callback );
    }
    ~barrier()
    {}

    barrier( const barrier& ) = delete;
    barrier( barrier&& ) = delete;
    barrier& operator=( barrier&& ) = delete;
    barrier& operator=( const barrier& ) = delete;

public:
    void wait()
    {
        std::unique_lock<std::mutex> lock( mutex_ );
        auto waiting_current = waiting_count_;
        if ( --count_ == 0 )
        {
            callback_();
            ++waiting_count_;
            count_ = thread_count_;

            cv_.notify_all();
        }
        else
        {
            cv_.wait( lock, [ & ] { return waiting_current != waiting_count_; } );
        }
    }
private:
    mutable std::mutex mutex_;
    mutable std::condition_variable cv_;
private:
    uint16_t count_;
    const uint16_t thread_count_;
    uint16_t waiting_count_;
    callback_fn callback_;
};

} // namespace custom

int main()
try
{
    const auto workers = { "Anil", "Busara", "Carl" };
 
    auto on_completion = []() noexcept
    {
        // locking not needed here
        static auto phase = "... done\n" "Cleaning up...\n";
        std::cout << phase;
        phase = "... done\n";
    };

    custom::barrier sync_point( std::ssize(workers), on_completion );
 
    auto work = [&](std::string name)
    {
        std::string product = "  " + name + " worked\n";
        std::cout << product;  // ok, op<< call is atomic
        sync_point.wait();
 
        product = "  " + name + " cleaned\n";
        std::cout << product;
        sync_point.wait();
    };
 
    std::cout << "Starting...\n";
    std::vector<std::jthread> threads;
    threads.reserve(std::size(workers));
    for (auto const& worker : workers)
    {
        threads.emplace_back(work, worker);
    }
    return EXIT_SUCCESS;
}
catch( const std::exception &e )
{
    return EXIT_FAILURE;
}
catch ( ... )
{
    return EXIT_FAILURE;
}
