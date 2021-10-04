#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <threads.h>
#include <sys/time.h>

const char * PATH_CACHE_SIZE_L0_DATA = "/sys/bus/cpu/devices/cpu0/cache/index0/size";
const char * PATH_MATRIX_FIRST = "./first";
const char * PATH_MATRIX_SECOND = "./second";

typedef int Data;

struct timeval t0, t1, dt;

struct Matrix
{
	int height, width;
	Data** matr;
};

typedef struct Matrix Matrix;

struct Block
{
	int index_i, index_j;
};

typedef struct Block Block;

struct args
{
	Matrix* a;
	Matrix* b;
	Matrix* c;
	Block* blocks;
	int number_of_blocks, block_size, number_of_blocks_per_row;
};

Matrix* create_matrix(int height, int width)
{
	Matrix* m = (Matrix *) malloc(sizeof(Matrix));
	m->height = height;
	m->width = width;
	m->matr = (Data **) calloc(m->height, sizeof(Data *));
	for (int i = 0; i < m->height; i++)
		m->matr[i] = (Data *) calloc(m->width, sizeof(Data));
	return m;
}

void clear_matrix(Matrix *m)
{
	for (int i = 0; i < m->height; i++)
		free(m->matr[i]);
	free(m->matr);
	free(m);
}

Matrix* read_matrix(const char * path)
{
	FILE* file_matrix = fopen(path, "r");
	int height, width;
	if (NULL == file_matrix)
		printf("empty file\n");
	fscanf(file_matrix, "%d%d", &height, &width);
	Matrix* m = create_matrix(height, width);
	for (int i = 0; i < m->height; i++)
		for (int j = 0; j < m->width; j++)
			fscanf(file_matrix, "%d", &m->matr[i][j]);
	fclose(file_matrix);
	return m;
}

void print_matrix(Matrix m)
{
	for (int i = 0; i < m.height; i++)
	{
		for (int j = 0; j < m.width; j++)
			printf("%d ", m.matr[i][j]);
		printf("\n");
	}
}

int get_cache_size()
{
	FILE* file_cache_size = fopen(PATH_CACHE_SIZE_L0_DATA, "r");
	int cache_size;
	fscanf(file_cache_size, "%d", &cache_size);
	fclose(file_cache_size);
	return cache_size * 1024;
}

int get_block_size()
{
	int cache_size = get_cache_size();
	return (int) sqrt(cache_size / 2 / sizeof(Data)); 
	//return 8;
}

Matrix* multiplicate_matrix(Matrix* a, Matrix* b, int block_size, int index_i, int index_j, int index_k) // A_ik * B_kj
{
	Matrix* c = create_matrix(block_size, block_size);
	Data elem;
	for (int i = 0; i < c->height; i++)
		for (int k = 0; k < block_size; k++)
		{
			elem = a->matr[index_i*block_size + i][index_k*block_size + k];
			for (int j = 0; j < c->width; j++)
				c->matr[i][j] += elem * b->matr[index_k*block_size + k][index_j*block_size + j];
		}
	return c;
}

void add_block(Matrix* m, Matrix* block, int index_i, int index_j, int block_size)
{
	for (int i = 0; i < block_size; i++)
		for (int j = 0; j < block_size; j++)
			m->matr[index_i*block_size + i][index_j*block_size + j] += block->matr[i][j];
}

void Multiplicate_block_matrix(Matrix *a, Matrix *b, Matrix *c, int index_i, int index_j, int block_size, int number_of_blocks_per_line) //C_ij = sum(A_ik * B_kj)
{
	for (int k = 0; k < number_of_blocks_per_line; k++)
	{
		Matrix* block = multiplicate_matrix(a, b, block_size, index_i, index_j, k);
		add_block(c, block, index_i, index_j, block_size);
		free(block);
	}
}

int process_calculations(void* args)
{
	struct args *normal_args = (struct args *)args;
	for (int i = 0; i < normal_args->number_of_blocks; i++)
		Multiplicate_block_matrix(normal_args->a, normal_args->b, normal_args->c, normal_args->blocks[i].index_i, normal_args->blocks[i].index_j, normal_args->block_size, normal_args->number_of_blocks_per_row);
	free(args);
	return 0;
}

void blocks_multiplication(Matrix* a, Matrix* b, Matrix* c, int number_of_threads)
{
	int block_size = get_block_size();
	int number_of_blocks_per_row = (a->height / block_size);
	int number_of_blocks_per_column = (a->width / block_size);
	int number_of_blocks_per_thread = number_of_blocks_per_row * number_of_blocks_per_column / number_of_threads;
	thrd_t* threads = (thrd_t *) malloc(number_of_threads * sizeof(thrd_t));
	int index_i = 0, index_j = 0;
	Block** blocks = (Block **) malloc(number_of_threads * sizeof(Block *));
	for (int i = 0; i < number_of_threads; i++)
	{
		blocks[i] = (Block *) malloc(number_of_blocks_per_thread * sizeof(Block));
		for (int j = 0; j < number_of_blocks_per_thread; j++)
		{
			blocks[i][j].index_i = index_i;
			blocks[i][j].index_j = index_j;
			index_j++;
			if (index_j == number_of_blocks_per_row)
			{
				index_i++;
				index_j = 0;
			}
		}
		struct args *args = (struct args *) malloc(sizeof(struct args));
		args->a = a;
		args->b = b; 
		args->c = c;
		args->blocks = blocks[i];
		args->number_of_blocks = number_of_blocks_per_thread;
		args->block_size = block_size;
		args->number_of_blocks_per_row = number_of_blocks_per_row;
		thrd_create(&threads[i], process_calculations, (void *) args);
	}
	for (int i = 0; i < number_of_threads; i++)
		thrd_join(threads[i], NULL);
	free(threads);
	for (int i = 0; i < number_of_threads; i++)
		free(blocks[i]);
	free(blocks);
}

int main(int argc, const char* argv[])
{
	int number_of_threads;
	if (argc == 2)
		sscanf(argv[1], "%d", &number_of_threads);
	else
		number_of_threads = 1;
	Matrix* first = read_matrix(PATH_MATRIX_FIRST);
	Matrix* second = read_matrix(PATH_MATRIX_SECOND);
	//printf("First matrix:\n");
	//print_matrix(*first);
	//printf("Second matrix:\n");
	//print_matrix(*second);
	Matrix* result = create_matrix(first->height, second->width);
	gettimeofday(&t0, NULL);
	blocks_multiplication(first, second, result, number_of_threads);
	gettimeofday(&t1, NULL);
	timersub(&t1, &t0, &dt);
	//printf("Result matrix:\n");
	//print_matrix(*result);
	//printf("Time: %ld.%06ld\n", dt.tv_sec, dt.tv_usec);
	printf("%ld.%06ld\n", dt.tv_sec, dt.tv_usec);
	clear_matrix(first);
	clear_matrix(second);
	clear_matrix(result);
	return 0;
}
