/*
4.	Частотный анализатор текстов.
k “читающих” потоков считывают данные из файлов (формат файлов произвольный).
Поток-интерфейс отвечает за взаимодействие
с пользователем (командная строка или иной формат).
Пользователю доступны следующие команды:
вывести на экран 5 самых распространённых на данный момент букв;
вывести на экран вероятность появление буквы, введённой пользователем;
выдать три самые редкие буквы.
*/

#include <iostream>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <fstream>
#include <istream>
#include <sstream>
#include <initializer_list>
#include <set>
#include <algorithm>
#include <map>
#include <omp.h>

template<typename T>
class FrequencyAnalyzer
{
    struct Frequency
    {
        T value;
        uint64_t requency;
    };

public:
    FrequencyAnalyzer() = default;

    ~FrequencyAnalyzer() = default;

public:
    void insert( const T& value )
    {
    std::lock_guard<std::mutex> lock( mutex_ );
        ++map_[ value ];
        map_.insert( value );
    }

    template < typename U >
    void insert_iterable( const U& value )
    {
        std::lock_guard<std::mutex> lock( mutex_ );
        std::for_each( value.begin(), value.end(), [&]( const T& v ) { ++map_[ v ]; } );
    }

    double get_p_of_occurrence( const T& value )
    {
        uint64_t total = 0;
        std::for_each(
            map_.begin(),
            map_.end(),
            [&]( auto&& elem ) {
                total += elem.second;
            } );
        return map_[ value ] / static_cast<double>( total );
    }

    std::vector<Frequency> get_first_top( uint16_t count ) const
    {
        std::lock_guard<std::mutex> lock( mutex_ );
        if ( count > map_.size() )
        {
            count = map_.size();
        }

        auto ret = std::vector< Frequency >{};
        ret.reserve( count );
        auto it = map_.begin();
        int32_t counter = 0;
        while ( counter++ < count )
        {
            ret.push_back( { it->first, it->second } );
            ++it;
        }
        ret.resize( count );
        return ret;
    }

    std::vector<Frequency> get_last_top( uint16_t count ) const
    {
        std::lock_guard<std::mutex> lock( mutex_ );

        if ( count > map_.size() )
        {
            count = map_.size();
        }

        auto ret = std::vector< Frequency >{};
        ret.reserve( count );
        auto it = map_.rbegin();
        int32_t counter = 0;
        while ( counter++ < count )
        {
            ret.push_back( { it->first, it->second } );
            ++it;
        }
        ret.resize( count );
        return ret;
    }

private:
    std::map< T, uint64_t > map_;
    mutable std::mutex mutex_;
};

class FileReader
{
public:
    ~FileReader() = default;

    FileReader( FrequencyAnalyzer<char>& analyzer, std::vector< std::string >& filenames  )
    {

        auto checked_filenames = std::set< std::string >{ filenames.begin(), filenames.end() };

        auto threads = std::vector < std::jthread >{};
        for ( auto&& filename : checked_filenames )
        {
            threads.emplace_back(
                [&]
                {
                    auto file = std::ifstream{ filename, std::ios::in };
                    if ( !file )
                    {
                        std::cerr << "Error: \"" << filename << "\" was not open" << std::endl;
                    }
                    #ifdef INSERT_ITERABLE
                        auto out = std::ostringstream{};
                        out << file.rdbuf();
                        analyzer.insert_iterable( out.str() );
                    #else
                        char ch;
                        while ( file.get( ch ) )
                        {
                            analyzer.insert( ch );
                        }
                    #endif
                }
            );
        }
    }
};

void run()
{
    #ifdef INSERT_ITERABLE
        std::cout << "checking insert_iterable" << std::endl;
    #else
        std::cout << "checking single insert" << std::endl;
    #endif

    auto list_of_files = std::vector< std::string > { "./garbage.txt", "./garbage_2.txt" };
    auto analyzer = std::make_unique<FrequencyAnalyzer<char>>();
    FileReader { *analyzer, list_of_files };
    auto p_chars = analyzer->get_last_top( 2 );
    for( auto&& p_char : p_chars )
    {
        std::cout << p_char.value << " " << p_char.requency << std::endl;
    }
    std::cout << "Done\n";

    p_chars = analyzer->get_first_top( 2 );
    for( auto&& p_char : p_chars )
    {
        std::cout << p_char.value << " " << p_char.requency << std::endl;
    }
    std::cout << "Done\n";
}

int main() try {
    run();
    return EXIT_SUCCESS;
}
catch (const std::exception& e) {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
}
catch (...) {
    std::cerr << "Unknown error" << "\n";
    return EXIT_FAILURE;
}
