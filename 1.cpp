#include <iostream>
#include <algorithm>
#include <thread>
#include <vector>

#include <random>
#include <iterator>
#include <functional>
#include <utility>

namespace
{

template < typename Container >
void print_arr ( const Container& container )
{
    std::for_each( container.begin(), container.end(), []( auto&& value ) { std::cout << value << " | "; } );
    std::cout << std::endl;
}

template < typename It >
void fill_w_rand( It&& begin, It&& end )
{
    std::for_each( begin, end,
    [] ( auto& elem )
    {
        elem = std::rand() % 15;
    } );
};

template < typename It >
void inc_n_check( It&& begin, It&& end )
{
    std::for_each( begin, end,
    [] ( auto& elem )
    {
        if ( ++elem > 1 )
        {
            std::cerr << " Warn: \trewriting in array" << std::endl;
        }
    } );
};

void fill_w_rand_n_check(
    std::pair<std::vector<int>::iterator, std::vector<int>::iterator> begin,
    std::pair<std::vector<int>::iterator, std::vector<int>::iterator> end )
{
    inc_n_check( begin.second, end.second );
    fill_w_rand( begin.first, end.first );
}

std::vector< int32_t> get_values( std::size_t arr_size, int32_t thread_count )
{
    if ( thread_count > arr_size || arr_size <=0 || thread_count <= 0 )
    {
        std::cerr << "Bad args " << std::endl;
        return {};
    }

    auto arr_rand = std::vector< int32_t > ( arr_size, 0 );
    auto arr_counter = std::vector< int32_t > ( arr_size, 0 );

    auto rng = [] { return std::rand() % 5 + 1; };

    auto arr_thread = std::vector< std::thread > ();
    auto check_arr = std::vector< int32_t > ( arr_size, 0 );

    int32_t per_count = arr_size / thread_count;
    int32_t latest = 0;

    for ( int i = 0; i < thread_count; ++i )
    {
        auto start_distance = std::exchange( latest, latest + per_count );
        arr_thread.push_back(
            std::thread{ [&, start_distance, per_count, latest]
        {
            // std::generate_n( arr_rand.begin() + start_distance,
            //        per_count, std::ref( rng ) );
            fill_w_rand_n_check(
                {  arr_rand.begin() + start_distance, arr_counter.begin() + start_distance },
                { arr_rand.begin() + latest,  arr_counter.begin() + latest } );
            #ifdef DEBUG
                print_arr( arr_rand );
                print_arr( arr_counter );
            #endif
        } }
        );
        #ifdef DEBUG
            std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
        #endif
    }

    auto tmp = arr_size - arr_size % thread_count;
    fill_w_rand_n_check( { arr_rand.begin() + tmp, arr_counter.begin() + tmp, }, {  arr_rand.begin() + tmp + ( arr_size % thread_count ),  arr_counter.begin() + tmp + ( arr_size % thread_count ) } );
    for ( auto& thread : arr_thread ) { if ( thread.joinable() ) thread.join(); }
    #ifdef DEBUG
        print_arr( arr_rand );
    #endif
    return arr_rand;
}

} // namespace

int main( int argc, char ** argv )
try
{
    srand(time(nullptr));
#ifdef DEBUG
    if ( argc < 3 )
    {
        std::cerr << "Usage: " << argv[0] << " <array_size> <thread_count>" << std::endl;
        return EXIT_FAILURE;
    }
    int32_t arr_size = std::stoi( argv[1] );
    int32_t thread_count = std::stoi ( argv[ 2 ]  );
    auto values = get_values( arr_size, thread_count );
#else
    while ( true )
    {
        int32_t arr_size = std::rand() % 1000 + 1;
        int32_t thread_count = std::rand() % 50 + 1;
        std::cout << "arr_size: " << arr_size << " | thread_count: " << thread_count << std::endl;
        auto values = get_values( arr_size, thread_count );
        std::cout << "OK" << std::endl;
        std::this_thread::sleep_for ( std::chrono::seconds ( 3 ) );
    }
#endif
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