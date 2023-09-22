 #include <iostream>
#include <vector>
#include <bitset>
#include <algorithm>
#include <ctime>
#include <random>
#include <cassert>
#include <mpi.h>
#include <thread>

constexpr auto bits_size = 8;
using bitset_t = std::vector<int32_t>;
using matrix_t = std::vector<bitset_t>;
using indexes_t = std::array<int32_t, 2>;

int get_random_int()
{
	std::random_device rd;
	std::uniform_int_distribution<int32_t> dist(0, 500);
	return dist( rd ) % 2 ? 1 : 0;
}

bool duplicateInSquare( const matrix_t& matrix, uint16_t i, uint16_t j )
{
	/*if ( !( matrix[i][j] ^ matrix[i + 1][j] ) &&
		!( matrix[i][j] ^ matrix[i][j + 1] ) &&
		!( matrix[i][j] ^ matrix[i + 1][j + 1] ) ) {
		return true;
	}*/

	if ( ( matrix[i][j] == matrix[i + 1][j] ) &&
		( matrix[i][j] == matrix[i][j + 1] ) &&
		( matrix[i][j] == matrix[i + 1][j + 1] ) ) {
			return true;
	}
	return false;
}

bool hasDuplicates( const matrix_t& matrix )
{
	for ( int i = 0; i < matrix.size() - 1; ++i ) {
		for ( int j = 0; j < bits_size - 1; ++j ) {
			if ( duplicateInSquare( matrix, i, j ) ) {
				return true;
			}
		}
	}
	return false;
}

void removeDuplicates( matrix_t& matrix, int32_t from, int32_t to, bool cout = false )
{
	if (to == matrix.size() ) {
		to = to - 1;
	}
	for ( int i = from; i < to; ++i ) {
		for ( int j = 0; j < 8; ++j ) {
			if ( duplicateInSquare( matrix, i, j ) ) {
				matrix[i + 1][j + 1] ^= 1;
			}
		}
	}
}

void removeDuplicatesInLast( matrix_t& matrix, int32_t from, int32_t to, bool cout = false )
{
	if (to == matrix.size() ) {
		to = to - 1;
	}
	for ( int i = from; i < to; ++i ) {
		for ( int j = 0; j < 8; ++j ) {
			if ( duplicateInSquare( matrix, i, j ) ) {
				matrix[i][j + 1] ^= 1;
			}
		}
	}
}

void removeDuplicatesInLast( matrix_t& matrix, int32_t row )
{

	for ( int j = 0; j < 8; ++j ) {
			if (  j % 2  ) {
				matrix[row][j] = 1;
			} else {
				matrix[row][j] = 0;
			}
	}
}

void print_matrix( matrix_t& matrix )
{
	std::for_each( matrix.begin(), matrix.end(), []( auto&& bits ) {
		std::for_each( bits.begin(), bits.end(), [] ( auto&& bit ) {
			std::cout << bit;
		} );
		std::cout << std::endl;
	} );
}

void start( int argc, char* argv[] )
{
	MPI_Init(&argc, &argv);
	constexpr auto rank_of_program = 0;
	int rank{};
	int comm_size{};
	// Получаем номер конкретного процесса на котором запущена программа
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	// Получаем количество запущенных процессов
	MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
	assert ( comm_size >= 2 );
	int32_t matrix_size = 18;

	if (rank == 0) {

		auto prepare_matrix = [&]
		{
			auto ret = std::vector<std::vector<int>>{};
			std::cout << "Matrix size: " << matrix_size << std::endl;
			ret.resize( matrix_size );
			for ( int i = 0; i < matrix_size; ++i ) {
				for ( int j = 0; j < bits_size; ++j ) {
					ret[i].push_back( get_random_int() );
				}
			}

			return ret;
		};

		/// 1 0 1 1 1
		/// 1 1 0 0 0
		/// 1 0 0 1 1
		/// 1 1 0 1 1
		/// 0 0 1 1 1
		/// 0 0 0 0 0

		auto send_n_recv = [&] ( const indexes_t& indexes, matrix_t& matrix, int32_t dist )
		{
			MPI_Send(indexes.data(), indexes.size(), MPI_INT, dist, 0, MPI_COMM_WORLD);

			for ( int j = indexes[0], counter = 0; counter < indexes[1]; ++counter, ++j )
			{
				MPI_Send(matrix[j].data(), bits_size, MPI_INT, dist, 0, MPI_COMM_WORLD);
			}
			for ( int j = indexes[0], counter = 0; counter < indexes[1]; ++counter, ++j )
			{
				MPI_Recv(matrix[j].data(), bits_size, MPI_INT, dist, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			}
		};

		auto matrix = prepare_matrix();
		print_matrix( matrix );

		std::cout << ( !hasDuplicates( matrix )? "[WARN]: No duplications" : "[INFO]: There are duplications!" ) << std::endl;
		// std::this_thread::sleep_for( std::chrono::seconds( 1 ) );

		std::cout << "Matrix successfully generated" << std::endl;
		std::cout << "Trying to send a matrix in pieces..." << std::endl;

		auto indexes = indexes_t{ 0, 0 };
		auto count_processors = comm_size - 1;
		indexes[1] = matrix_size / count_processors;
		send_n_recv( indexes, matrix, 1 );

		if ( matrix_size / count_processors != matrix_size ) {
			indexes[0] += ( indexes[1] );
			// indexes[1] += 1;
			for ( int i = 1; i < count_processors - 1 ; ++i )
			{
				send_n_recv( indexes, matrix, i + 1 );
				indexes[0] += ( indexes[1] );
			}

			indexes[1] = matrix_size / count_processors + matrix_size % count_processors;
			//indexes[1] += 1;
			send_n_recv( indexes, matrix, count_processors );
		}

		std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
		print_matrix( matrix );
		std::cout << ( !hasDuplicates( matrix )? "[INFO]: no duplications, congrats!" : "[ERROR]: There are still duplications in your matrix" ) << std::endl;
		std::cout << "Programm successfully finished" << std::endl;
	} else {

		auto prepare_matrix = [&]
		{
			auto ret = std::vector<std::vector<int>> {};
			ret.resize(matrix_size);
			for ( int i = 0; i < matrix_size; ++i )
			{
				ret[i].resize( bits_size );
			}
			return ret;
		};

		auto recv_indexes = [&]
		{
			auto ret = indexes_t {};
			MPI_Recv(ret.data(), ret.size(), MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			return ret;
		};

		auto recv_n_send = [] ( const indexes_t& indexes, matrix_t& matrix, const std::function<void( matrix_t& )>& callback )
		{
			for ( int i = indexes[0], counter = 0; counter < indexes[1]; ++counter, ++i )
			{
				MPI_Recv(matrix[i].data(), bits_size, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			}

			std::cout << "\n\n";
			print_matrix( matrix );
			std::cout << "\n\n";
			callback( matrix );

			for ( int i = indexes[0], counter = 0; counter < indexes[1]; ++counter, ++i )
			{
				MPI_Send(matrix[i].data(), bits_size, MPI_INT, 0, 0, MPI_COMM_WORLD);
			}
		};

		auto matrix = prepare_matrix();
		auto indexes = recv_indexes();
		recv_n_send( indexes, matrix, [&] ( matrix_t& matrix ) {
			removeDuplicates ( matrix, indexes[0], indexes[0] + indexes[1] - 1 );
			// removeDuplicatesInLast ( matrix, indexes[0] + indexes[1] - 1, indexes[0] + indexes[1] );
			removeDuplicatesInLast ( matrix, indexes[0] + indexes[1] - 1 );
		} );

	}
	MPI_Finalize();
}

int main(int argc, char *argv[]) try {
	start( argc, argv );
	return EXIT_SUCCESS;
} catch( const std::exception& e ) {
	std::cerr << "error: " << e.what() << std::endl;
	return EXIT_SUCCESS;
} catch ( ... ) {
	std::cerr << "unknown" << std::endl;
	return EXIT_FAILURE;
}
