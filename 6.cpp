#include <iostream>
#include <algorithm>
#include <vector>
#include <random>
#include <chrono>
#include <cassert>
#include <functional>
#include <omp.h>

namespace
{

using matrix_t = std::vector<std::vector<int32_t>>;

void fill_matrix( matrix_t& matrix, uint16_t rows, uint16_t columns )
{
    assert( columns > 0 && rows > 0 );

    matrix.resize( rows );
    std::random_device  rd;
    std::uniform_int_distribution<int32_t> dist(1, 10);

    std::for_each( matrix.begin(), matrix.end(), [&]( auto&& col )
    {
        col.resize( columns );
        std::for_each( col.begin(), col.end(), [&]( auto& val )
        {
            val = dist(rd);
        } );
    } );
}

void fill_matrix( matrix_t& matrix, int32_t to_fill,  uint16_t rows, uint16_t columns )
{
    assert( columns > 0 && rows > 0 );

    matrix.resize( rows );
    std::for_each( matrix.begin(), matrix.end(), [&]( auto&& col )
    {
        col.resize( columns );
        std::for_each( col.begin(), col.end(), [&]( auto& val )
        {
            val = to_fill;
        } );
    } );
}

void print_matrix( const matrix_t& matrix )
{
    std::for_each( matrix.begin(), matrix.end(), []( auto&& col )
    {
        std::for_each( col.begin(), col.end(), []( auto&& val )
        {
            std::cout << val << ' ';
        } );
        std::cout << std::endl;
    } );
}

bool check_eq( const matrix_t& a, const matrix_t& b )
{
    assert( a.size() == b.size() );
    for ( int i = 0; i < b.size() ; ++ i)
    {
        for ( int j = 0; j < b.size(); ++ j)
        {
            assert( a[j].size() == b[j].size() );
            if ( a[i][j] != b[i][j] )
            {
                return false;
            }
        }
    }
    return true;
}   

matrix_t mult_matrix_no_threads( const matrix_t& a, const matrix_t& b )
{
    auto a_columns = ( * a.begin() ).size();
    auto b_rows = b.size();
    assert( a_columns == b_rows );

    auto result = matrix_t{};
    fill_matrix( result, 0, a.size(), ( * b.begin() ).size() );
    std::cout << "No threads" << std::endl;
    for ( std::size_t i = 0; i < a.size(); ++i )
    {
        for ( std::size_t j = 0; j < ( * b.begin() ).size(); ++j )
        {
            for ( std::size_t k = 0; k < ( * a.begin() ).size(); ++ k )
            {
                result[i][j] += a[i][k] * b[k][j];
            }
        }
    }
    return result;
}

matrix_t mult_matrix( const matrix_t& a, const matrix_t& b )
{
    auto a_columns = ( * a.begin() ).size();
    auto b_rows = b.size();
    assert( a_columns == b_rows );

    auto result = matrix_t{};
    fill_matrix( result, 0, a.size(), ( * b.begin() ).size() );
    std::cout << "With threads" << std::endl;
    #pragma omp parallel for num_threads(10)
    for ( std::size_t i = 0; i < a.size(); ++i )
    {
        for ( std::size_t j = 0; j < ( * b.begin() ).size(); ++j )
        {
            for ( std::size_t k = 0; k < ( * a.begin() ).size(); ++ k )
            {
                result[i][j] += a[i][k] * b[k][j];
            }
        }
    }

    return result;
}

matrix_t mult_matrix_IJK( const matrix_t& a, const matrix_t& b )
{
    auto a_columns = ( * a.begin() ).size();
    auto b_rows = b.size();
    assert( a_columns == b_rows );

    auto result = matrix_t{};
    fill_matrix( result, 0, a.size(), ( * b.begin() ).size() );

    std::cout << "With threads" << std::endl;
    constexpr auto thread_num = 10;

    #pragma omp parallel for num_threads(thread_num)

    for ( std::size_t i = 0; i < a.size(); ++i )
    {
        for ( std::size_t j = 0; j < ( * b.begin() ).size(); ++j )
        {
            for ( std::size_t k = 0; k < ( * a.begin() ).size(); ++ k )
            {
                result[i][j] += a[i][k] * b[k][j];
            }
        }
    }

    return result;
}

matrix_t mult_matrix_JIK( const matrix_t& a, const matrix_t& b )
{
    auto a_columns = ( * a.begin() ).size();
    auto b_rows = b.size();
    assert( a_columns == b_rows );

    auto result = matrix_t{};
    fill_matrix( result, 0, a.size(), ( * b.begin() ).size() );

    std::cout << "With threads" << std::endl;
    constexpr auto thread_num = 10;

    #pragma omp parallel for num_threads(thread_num)

    for ( std::size_t j = 0; j < ( * b.begin() ).size(); ++j )
    {
        for ( std::size_t i = 0; i < a.size(); ++i )
        {
            for ( std::size_t k = 0; k < ( * a.begin() ).size(); ++ k )
            {
                result[i][j] += a[i][k] * b[k][j];
            }
        }
    }

    return result;
}

} // namespace

int main() try
{
    std::cout << "Running a program" << std::endl;
    auto first_matrix = matrix_t{};
    auto second_matrix = matrix_t{};
    fill_matrix( first_matrix, 1000, 1000 );
    #ifdef DEBUG
    print_matrix( first_matrix );
    std::cout << "----------" << std::endl;
    #endif
    fill_matrix( second_matrix, 1000, 1000 );
    #ifdef DEBUG
    print_matrix( second_matrix );
    std::cout << "----------" << std::endl;
    #endif
    auto time_count = [] ( const std::function< matrix_t(const matrix_t&, const matrix_t &) >& action,
        const matrix_t& a, const matrix_t& b )
    {
        auto start = std::chrono::steady_clock::now();
        auto res = action( a, b );
        auto end = std::chrono::steady_clock::now();
        std::cout << "Elapsed time in milliseconds: "
        << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
        << " ms" << std::endl;
        return res;
    };

    auto res_0 = time_count( mult_matrix_no_threads, first_matrix, second_matrix );
    auto res_1 = time_count( mult_matrix_IJK, first_matrix, second_matrix );
    auto res_2 = time_count (mult_matrix_JIK, first_matrix, second_matrix );

    std::cout << "Is eq?\t" << std::boolalpha << check_eq( res_0, res_1 ) << std::endl;
    std::cout << "Is eq?\t" << std::boolalpha << check_eq( res_0, res_2 ) << std::endl;
    #ifdef DEBUG
    std::cout << "--" << std::endl;
    #endif


}
catch(const std::exception& e)
{
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
}
catch( ... )
{
    std::cerr << "unknown exception" << std::endl;
    return EXIT_FAILURE;
}