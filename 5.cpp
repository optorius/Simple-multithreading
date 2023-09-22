// 1.	Реализовать скалярное произведение векторов (Reduction)
#include <iostream>
#include <vector>
#include <cassert>
#include <omp.h>
#include <limits>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <random>

namespace
{

int32_t dot_product( const std::vector<int32_t>& a, const std::vector<int32_t>& b )
{
    assert ( a.size() == b.size() );
    int32_t sum {};
    #pragma omp parallel for reduction(+:sum)
    for (int i = 0; i < a.size(); ++i)
    {
        sum += a[i] * b[i];
    }
    return sum;
}

void fill_w_rand( std::vector<int32_t>& arr, std::size_t size )
{
    for ( int i = 0; i < size; ++i)
    {
        arr.emplace_back( std::rand() % 10 );
    }
}
} // namespace

int main()
{
    int n;
    std::cout << "Enter size of vector" << std::endl;
    while ( !( std::cin >> n ) )
    {
        std::cin.clear();
        std::cin.ignore( std::numeric_limits<std::streamsize>::max(), '\n' );
    }

    auto a = std::vector<int32_t>();
    auto b = std::vector<int32_t>();
    fill_w_rand( a, n );
    fill_w_rand( b, n );
    #ifdef DEBUG
        auto print = [] ( const std::vector<int32_t>& arr )
        {
            std::for_each( arr.begin(), arr.end(), [] ( auto&& val ) { std::cout << val << " "; } );
            std::cout << std::endl;
        };
        print( a );
        print( b );
    #endif

    auto res = dot_product(a , b);
    std::cout << "Result of sum:\t" << res << std::endl;
    return 0;
}