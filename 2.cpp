#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace custom
{

class thread_joiner
{
public:
    /// @brief Конструктор
    /// @param thread захватываемый поток
    explicit thread_joiner( std::thread&& thread )
        : thread_( std::move( thread ) )
    {
    }

    /// @brief Деструктор
    /// Вызов join
    ~thread_joiner()
    {
        if (thread_.joinable())
        {
            thread_.join();
        }
    }

    /// @return Захваченный поток
    std::thread& get() { return thread_; }

private:
    std::thread thread_;
};

template <
    class T,
    class Container = std::deque<T>
>
class tsafe_queue
{
public: // special functions
    tsafe_queue() = default;

    ~tsafe_queue() = default;

    explicit tsafe_queue( const Container& cont )
        : queue_( cont )
    {
    }

    explicit tsafe_queue( Container&& cont )
        : queue_( std::move( cont ) )
    {
    }

    tsafe_queue( const tsafe_queue& other ) = delete;
    tsafe_queue( tsafe_queue&& other ) = delete;

public: // operator=
    tsafe_queue& operator=( const tsafe_queue& other ) = delete;
    tsafe_queue& operator=( tsafe_queue&& other ) = delete;

public: // element access
    bool front( T& out ) const
    {
        std::lock_guard<std::mutex> lock( mutex_ );
        if ( queue_.empty() )
        {
            ( void ) out;
            return false;
        }
        out = queue_.front();
        return true;
    }

    T front() const
    {
        std::unique_lock<std::mutex> lock( mutex_ );
        cv_.wait( lock, [ this ] { return !queue_.empty(); } );
        return queue_.front();
    }

    bool back( T& out ) const
    {
        std::lock_guard<std::mutex> lock( mutex_ );
        if ( queue_.empty() )
        {
            ( void ) out;
            return false;
        }
        out = queue_.back();
        return true;
    }

    T back() const
    {
        std::unique_lock<std::mutex> lock( mutex_ );
        cv_.wait( lock, [ this ] { return !queue_.empty(); } );
        return queue_.back();
    }

public: // capacity
    std::size_t size() const
    {
        std::lock_guard<std::mutex> lock( mutex_ );
        return queue_.size();
    }

    bool empty() const
    {
        std::lock_guard<std::mutex> lock( mutex_ );
        return queue_.empty();
    }

public: // modifiers
    void wait_n_pop()
    {
        std::unique_lock<std::mutex> lock( mutex_ );
        cv_.wait( lock, [ this ] { return !queue_.empty(); } );
        queue_.pop();
    }

    bool try_pop()
    {
        std::lock_guard<std::mutex> lock( mutex_ );
        if ( queue_.empty() )
        {
            return false;
        }
        queue_.pop();
        return true;
    }

    template <class... Args>
    void emplace( Args&&... args )
    {
        std::lock_guard<std::mutex> lock( mutex_ );
        queue_.emplace( args... );
        cv_.notify_one();
    }

    void push( const T& value )
    {
        std::lock_guard<std::mutex> lock( mutex_ );
        queue_.push( value );
        cv_.notify_one();
    }

    void push( T&& value )
    {
        std::lock_guard<std::mutex> lock( mutex_ );
        queue_.push( std::move( value ) );
        cv_.notify_one();
    }

private: // sync
    mutable std::mutex mutex_;

    mutable std::condition_variable cv_;

private:
    std::queue<T, Container> queue_;
};

} // namespace custom


void stdout_queue( custom::tsafe_queue<int32_t> & q )
{
    while ( !q.empty() )
    {
        std::cout << q.front() << std::endl;
        q.try_pop();
    }
}

void run_test_case()
{
    custom::tsafe_queue<int32_t> tsafe_queue;

    custom::thread_joiner t (  std::thread { [&tsafe_queue] {
        int32_t back_val = 0;
        std::cout << std::boolalpha << tsafe_queue.front( back_val ) << std::endl;
        std::cout << back_val << std::endl;
        std::cout << tsafe_queue.front() << std::endl;
    } } );

    custom::thread_joiner { std::thread( [&tsafe_queue] {
        std::cout << std::boolalpha << tsafe_queue.try_pop() << std::endl;
    }) };

    custom::thread_joiner { std::thread ( [&tsafe_queue] {
        tsafe_queue.push( 1 );
    }) } ;

    custom::thread_joiner { std::thread ( [&tsafe_queue] {
        tsafe_queue.push( 3 );
        tsafe_queue.push( 2 );
    } ) } ;
    std::thread x ( [&tsafe_queue] {
        std::cout << "setting to back" << std::endl;
        std::cout << "tsafe_queue.back() : " << tsafe_queue.back() << std::endl;
    });

    x.join();
    std::cout << "stdout queque" << std::endl;
    stdout_queue( tsafe_queue );
    std::cout << "another_tsafe_queue" << std::endl;
}

int32_t main()
try
{
    std::queue<int32_t> q;
    run_test_case();
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
