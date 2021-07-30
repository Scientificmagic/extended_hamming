/*      Extended Hamming Code Encoder/Decoder
    3Blue1Brown
    Hamming codes and error correction
    https://www.youtube.com/watch?v=X8jsijhllIA
    Hamming codes part 2, the elegance of it all
    https://www.youtube.com/watch?v=b3NxrZOu_CE

    Example 4x4 block
    +---------------+
    | E | P | P | x |
    | P | x | x | x |
    | P | x | x | x |
    | x | x | x | x |
    +---------------+
    E = Extended parity bit
        -> Detects if double-error occurs in a block
    P = Parity bits
        -> Block uses log2(size) parity bits
    x = data bits

    Output Format per block
    EPPPPxxxx...
        -> Gives flexability for padding
*/
/*      CLI Flags
    -a      -> all information
    -d/-e   -> decode/encode
    -h      -> help
    -o      -> output filename
    -q      -> quiet
    -s      -> size of matrix
    -v      -> vegetarian
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

const int MATRIX_SIZE_DEFAULT = 4;
const int CHARBITS = 8;

struct Settings
{
    int all;
    int decode;
    int encode;
    int help;
    int quiet;
    int matrix_size;
    int vegetarian;
};
struct Buffer
{
    int *buffer;
    int size;
    int capacity;
};
struct Matrix
{
    int **matrix;
    int dimension;
    int capacity;
};

// globals
struct Settings *settings;
FILE *input_file;
FILE *output_file;

// prototypes
void settings_initialize();
struct Buffer *buffer_initialize(int n);
struct Matrix *matrix_initialize(int dimension);

char *output_filename_append(char *input_filename, int e_flag, int* free_me);
void charToBinary(char c, struct Buffer *buffer);
char binaryToChar(struct Buffer *binary);

void buffer_enqueue(struct Buffer *buffer, int d);
int buffer_dequeue(struct Buffer *buffer);
void matrix_print(struct Matrix *matrix);

void encode_matrix_fill(struct Matrix *matrix, struct Buffer *buffer);
void encode_matrix_parity(struct Matrix *matrix);
void encode_matrix_write(struct Matrix *matrix);

void decode_matrix_fill(struct Matrix *matrix);
void decode_matrix_parity(struct Matrix *matrix, int *errors, int *correctable);
void decode_matrix_write(struct Matrix *matrix, struct Buffer *buffer);

void encode();
void decode();
void encode_vegetarian();
void decode_vegetarian();

void print_usage();
void print_all();
void print_size();


int main(int argc, char **argv)
{
    if (argc == 1)
        print_usage();

    settings_initialize();

    int opt;
    char *input_filename = NULL;
    char *output_filename = NULL;
    // handle flags
    while ((opt = getopt(argc, argv, "adeho:qs:v" )) != -1)
    {
        switch (opt)
        {
            case 'a':
                settings->all = 1;
                break;
            case 'd':
                settings->decode = 1;
                // if (settings->encode)
                //     print_usage();
                break;
            case 'e':
                settings->encode = 1;
                // if (settings->decode)
                //     print_usage();
                break;
            case 'h':
                settings->help = 1;
                break;
            case 'o':
                output_filename = optarg;
                break;
            case 'q':
                settings->quiet = 1;
                break;
            case 's':
                settings->matrix_size = atoi(optarg);
                // printf("Matrix size changed to: %d\n", settings->matrix_size);
                break;
            case 'v':
                settings->vegetarian = 1;
                // printf("Vegetarian mode: %d\n", settings->vegetarian);
                break;
            default:
                fprintf(stderr, "Error: defaulted in getopt switch case\n");
                exit(1);
        }
    }
    // handle settings
    if (settings->help == 1)
        print_usage();
    if (settings->all == 1)
        print_all();
    if (!(settings->matrix_size >= 2) || !(settings->matrix_size <= 256))
        print_size();
    if ((settings->matrix_size & (settings->matrix_size - 1)) != 0)
        print_size();
    // if both -e and -d are set/unset, exit
    if ((settings->encode ^ settings->decode) == 0)
        print_usage();

    // handle filenames
    if((input_filename = argv[optind]) == NULL)
        print_usage();
    int free_me = 0;
    if(output_filename == NULL)
    {
        output_filename = output_filename_append(input_filename, settings->encode, &free_me);
    }

    // handle files
    input_file = fopen(input_filename, "r");
    output_file = fopen(output_filename, "w");
    // printf("Input filename is: %s\n", input_filename);
    // printf("Output filename is: %s\n", output_filename);
    if (free_me)
        free(output_filename);
    if (input_file == NULL || output_file == NULL)
    {
        fprintf(stderr, "Error: Invalid filename\n");
        exit(1);
    }

    if (settings->encode)
    {
        if (settings->vegetarian)
            encode_vegetarian();
        else
            encode();
    }
    else if (settings->decode)
    {
        if (settings->vegetarian)
            decode_vegetarian();
        else
            decode();
    }

    fflush(output_file);

    fclose(input_file);
    fclose(output_file);
    free(settings);
    return 0;
}


void settings_initialize()
{
    settings = calloc(1, sizeof(struct Settings));
    settings->matrix_size = MATRIX_SIZE_DEFAULT;
}
struct Buffer *buffer_initialize(int n)
{
    struct Buffer *rtrn = calloc(1, sizeof(struct Buffer));
    rtrn->buffer = calloc(n, sizeof(int));
    rtrn->size = 0;
    rtrn->capacity = n;
    return rtrn;
}
struct Matrix *matrix_initialize(int dimension)
{
    struct Matrix *rtrn = calloc(1, sizeof(struct Matrix));
    rtrn->dimension = dimension;
    rtrn->capacity = dimension * dimension;

    rtrn->matrix = malloc(dimension * sizeof(int *));
    for (int i = 0; i < dimension; i++)
    {
        rtrn->matrix[i] = calloc(dimension, sizeof(int));
    }
    return rtrn;
}

char* output_filename_append(char *input_filename, int e_flag, int *free_me)
{
    // copy input into filename
    int len = strlen(input_filename) + 9;
    char *filename = malloc(len * sizeof(char));
    strcpy(filename, input_filename);
    
    // store extension separate and strip extension from filename
    char *extension = NULL;
    char *dot = strrchr(filename, '.');
    if (dot != NULL)
    {
        extension = strdup(dot);
        *dot = '\0';
    }

    // append proper naming to filename
    if (e_flag)
        strcat(filename, "_encoded");
    else
        strcat(filename, "_decoded");

    // if there was an extension, add it back
    if (extension != NULL)
    {
        strcat(filename, extension);
        free(extension);
    }

    *free_me = 1;
    return filename;
}
void charToBinary(char c, struct Buffer *buffer)
{
	for (int i = 0; i < CHARBITS; i++)
		buffer->buffer[buffer->size++] = c >> (CHARBITS - 1 - i) & 1;
}
char binaryToChar(struct Buffer *binary)
{   // takes binary array[8], and converts to char
    char c = '\0';
    for (int i = 0; i < CHARBITS; i++)
    {
        c |= binary->buffer[i] << (CHARBITS - 1 - i);
    }
    return c;
}

void buffer_enqueue(struct Buffer *buffer, int d)
{
    buffer->buffer[buffer->size++] = d;
}
int buffer_dequeue(struct Buffer *buffer)
{   // buffer->size is being used a dequeue index
    // so if buffer's empty...
    if (buffer->size == buffer->capacity)
    {
        char c = fgetc(input_file);

		if (c == EOF)
		{	// return padding
            return -1;
        }

        // zero size for proper enqueue
        // then zero again for proper dequeue
        buffer->size = 0;
        charToBinary(c, buffer);
        buffer->size = 0;
    }

    return buffer->buffer[buffer->size++];
}
void matrix_print(struct Matrix *matrix)
{   // i -> horizontal row; j -> vertical column
	for (int i = 0; i < matrix->dimension; i++)
	{
		for (int j = 0; j < matrix->dimension; j++)
		{
			printf("%3d ", matrix->matrix[i][j]);
			if (j == matrix->dimension - 1)
				printf("\n");
		}		
	}
	printf("\n");
}

void encode_matrix_fill(struct Matrix *matrix, struct Buffer *buffer)
{   // i -> horizontal row; j -> vertical column
	for (int i = 0; i < matrix->dimension; i++)
	{
		for (int j = 0; j < matrix->dimension; j++)
		{	// if a power of 2 and first row/column, skip
            // [0][0] also skipped. That's desirable
			if (i == 0 && (j & (j-1)) == 0)
			{
				matrix->matrix[i][j] = 0;
				continue;
			}
			if (j == 0 && (i & (i-1)) == 0)
			{
				matrix->matrix[i][j] = 0;
				continue;				
			}

			matrix->matrix[i][j] = buffer_dequeue(buffer);
		}
	}
}
void encode_matrix_parity(struct Matrix *matrix)
{   // go through matrix and XOR all indices where there's a 1
	// ignore 0's and -1's
	int parity = 0;
	int metaParity = 0;
	for (int i = 0; i < matrix->dimension; i++)
	{
		for (int j = 0; j < matrix->dimension; j++)
		{
			if (matrix->matrix[i][j] == 1)
			{	// there's a 1 in the matrix location
				parity ^= (i * matrix->dimension) + j;
				metaParity ^= 1;
			}				
		}
	}

	// after calculating parity, set the bits
	int intBits = sizeof(int) * 4;
	int position, x, y;
	for (int i = 0; i < intBits; i++)
	{	// if bit shift by i is == 1
		if (((parity >> i) & 1) == 1)
		{
			metaParity ^= 1;
			position = 1 << i;
			x = position / matrix->dimension; // row
			y = position % matrix->dimension; // col
			matrix->matrix[x][y] = 1;
		}
	}

	// finally set the meta parity bit at [0][0]
	if (metaParity == 1)
	{
		matrix->matrix[0][0] = 1;
	}
}
void encode_matrix_write(struct Matrix *matrix)
{
    // for [0][j], write parity bits
    // [0][0] also gets written
    for (int j = 0; j < matrix->dimension; j++)
    {
        if ((j & (j-1)) == 0)
        {
            fprintf(output_file, "%d", matrix->matrix[0][j]);
        }
    }
    // for [i][0], write parity bits
    // make sure to skip [0][0] in this loop
    for (int i = 1; i < matrix->dimension; i++)
    {
        if ((i & (i-1)) == 0)
        {
            fprintf(output_file, "%d", matrix->matrix[i][0]);
        }
    }

    // write normal data, skipping parity bits
	for (int i = 0; i < matrix->dimension; i++)
	{
		for (int j = 0; j < matrix->dimension; j++)
		{   // if parity bit, don't write
            if (i == 0 && (j & (j-1)) == 0)
			{
				continue;
			}
			if (j == 0 && (i & (i-1)) == 0)
			{
				continue;				
			}
            // if padding bit, we're done writing
			if (matrix->matrix[i][j] == -1)
			{
                return;
            }

			fprintf(output_file, "%d", matrix->matrix[i][j]);
		}
	}
}

void decode_matrix_fill(struct Matrix *matrix)
{   
    // read parity bits [0][j]
    for (int j = 0; j < matrix->dimension; j++)
    {
        if ((j & (j-1)) == 0)
        {
            fscanf(input_file, "%1d", &matrix->matrix[0][j]);
        }
    }
    // read parity bits [i][0], skipping [0][0]
    for (int i = 1; i < matrix->dimension; i++)
    {
        if ((i & (i-1)) == 0)
        {
            fscanf(input_file, "%1d", &matrix->matrix[i][0]);
        }
    }

    // read normal data, skipping parity bits
	for (int i = 0; i < matrix->dimension; i++)
	{
		for (int j = 0; j < matrix->dimension; j++)
		{   // if parity bit, don't write
            if (i == 0 && (j & (j-1)) == 0)
			{
				continue;
			}
			if (j == 0 && (i & (i-1)) == 0)
			{
				continue;				
			}

			fscanf(input_file, "%1d", &matrix->matrix[i][j]);
            if (feof(input_file))
            {
                matrix->matrix[i][j] = -1;
            }
		}
	}
}
void decode_matrix_parity(struct Matrix *matrix, int *errors, int *correctable)
{   // xor wherever there's a 1 in the matrix
    // xor should result in binary 00000000 if everything's ok
    // else, the binary result will be the index of the bit error
    int parity = 0;
	int metaParity = 0;
	for (int i = 0; i < matrix->dimension; i++)
	{
		for (int j = 0; j < matrix->dimension; j++)
		{   // don't include metaParity bit
            if (i == 0 && j == 0)
            {
                continue;
            }

			if (matrix->matrix[i][j] == 1)
			{	// there's a 1 in the matrix location
				parity ^= (i * matrix->dimension) + j;
				metaParity ^= 1;
			}				
		}
	}
    // printf("parity: %d\n", parity);
    if (parity != 0)
    {   // if there's something wrong, but metaParity matches,
        // then there is 2+ errors and it's unfixable
        if (metaParity == matrix->matrix[0][0])
        {
            *errors += 2;
            *correctable = 0;
            return;
        }
        // else only 1 error
        else
        {   // bitflip the index of the parity variable
            int x = parity / matrix->dimension; // row
			int y = parity % matrix->dimension; // col
            matrix->matrix[x][y] ^= 1;

            *errors += 1;
            return;
        }
    }
}
void decode_matrix_write(struct Matrix *matrix, struct Buffer *buffer)
{   // enqueue data bits into buffer
    // then convert binary to char to write to file
    for (int i = 0; i < matrix->dimension; i++)
    {
        for (int j = 0; j < matrix->dimension; j++)
        {   // don't enqueue parity bits
            if (i == 0 && (j & (j-1)) == 0)
			{
				continue;
			}
			if (j == 0 && (i & (i-1)) == 0)
			{
				continue;				
			}

            // ignore padding
            if (matrix->matrix[i][j] != -1)
            {
                buffer_enqueue(buffer, matrix->matrix[i][j]);
            }

            if (buffer->size == buffer->capacity)
            {
                char c = binaryToChar(buffer);
                buffer->size = 0;
                fprintf(output_file, "%c", c);
            }
        }
    }
}

void encode()
{
    struct Buffer *buffer = buffer_initialize(8);
    struct Matrix *matrix = matrix_initialize(settings->matrix_size);

    // size = capacity to make sure the buffer gets filled initially
    buffer->size = buffer->capacity;

    // stop when EOF and buffer's depleted
    // using buffer->size as dequeue index
    while (!feof(input_file) || buffer->size != buffer->capacity)
    {
        encode_matrix_fill(matrix, buffer);
        // matrix_print(matrix);
        encode_matrix_parity(matrix);
        // matrix_print(matrix);
        encode_matrix_write(matrix);
    }

    if (settings->quiet == 0)
    {
        int numParity = 0;
        for (int i = 0; i < matrix->dimension; i++)
        {
            if ((i & (i -1)) == 0)
                numParity++;
        }
        numParity = numParity * 2 - 1;
        float redundancy = (float)numParity / matrix->capacity * 100;
        printf("%d parity bits / %d bit block = %2.2f%% redundancy\n",
            numParity, matrix->capacity, redundancy);
    }


    free(buffer->buffer);
    free(buffer);
    free(matrix->matrix);
    free(matrix);
}
void decode()
{
    struct Matrix *matrix = matrix_initialize(settings->matrix_size);
    struct Buffer *buffer = buffer_initialize(8);
    int errors = 0;
    int correctable = 1;

    while (!feof(input_file))
    {
        decode_matrix_fill(matrix);
        decode_matrix_parity(matrix, &errors, &correctable);
        decode_matrix_write(matrix, buffer);
    }

    if (settings->quiet == 0)
    {
        if (errors != 0)
        {
            if (correctable == 1)
            {
                printf("%d bit error(s) detected.\n", errors);
                printf("All errors corrected.\n");
            }
            else
            {
                printf("%d or more bit error(s) detected.\n", errors);
                printf("Not all errors could be corrected.\n");
            }   
        }
        else
        {
            printf("No errors detected.\n");
        }
    }
    
    free(buffer->buffer);
    free(buffer);
    free(matrix->matrix);
    free(matrix);
}
void encode_vegetarian()
{
    struct Buffer *buffer = buffer_initialize(8);

    char c;
    while ((c = fgetc(input_file)) != EOF)
    {
        charToBinary(c, buffer);

        for (int i = 0; i < 8; i++)
        {
            fprintf(output_file, "%d", buffer->buffer[i]);
        }
        buffer->size = 0;
    }

    free(buffer->buffer);
    free(buffer);
}
void decode_vegetarian()
{
    struct Buffer *buffer = buffer_initialize(8);

    while (!feof(input_file))
    {   // read 1 binary integer at a time
        for (int i = 0; i < 8; i++)
        {
            fscanf(input_file, "%1d ", &buffer->buffer[i]);
        }

        char c = binaryToChar(buffer);
        fprintf(output_file, "%c", c);
    }

    free(buffer->buffer);
    free(buffer);
}

void print_usage()
{
        printf("Usage:\n");
        printf("\thamming <input> -e/-d [option]...\n");
        printf("\t-a for all options\n");
        exit(2);
}
void print_all()
{
    printf("All options:\n");
    printf("\t-a\n\t\tprint all options\n");
    printf("\t-d\\-e\n\t\tdecode or encode file\n");
    printf("\t-h\n\t\tprint usage\n");
    printf("\t-o\n\t\toutfile name\n");
    printf("\t-q\n\t\tquiet\n");
    printf("\t-s\n\t\tsize of matrix (default 4)\n");
    printf("\t\t\trange of [2, 256]\n");
    printf("\t\t\tmust be a power of 2\n");
    printf("\t-v\n\t\tvegetarian\n");
    exit(2);
}
void print_size()
{
    printf("Size must be in range [2, 256] and a power of 2\n");
    exit(2);
}