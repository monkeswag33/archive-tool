#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "global.h"
// Help message
const char* help = "\
unarchiver <filename>\
";

// Read input file header, directory namd and number of files
void read_header(int fd, char* directory_name, uint32_t* num_files) {
	lseek(fd, 0, SEEK_SET);
	char* header = (char*) malloc(HEADER_SIZE);
	if (read(fd, header, HEADER_SIZE) == -1)
		error("Error reading file");
	memcpy(directory_name, header + DIRECTORY_NAME_OFFSET, ENTRY_NAME_SIZE);
	memcpy(num_files, header + FILE_COUNT_OFFSET, FILE_COUNT_SIZE);
	free(header);
	header = NULL;
}

// Read file header (filename and file length)
void read_file_header(int fd, uint32_t offset, char* filename, uint32_t* file_length) {
	lseek(fd, offset, SEEK_SET);
	char* header = (char*) malloc(FILE_HEADER_SIZE);
	if (read(fd, header, FILE_HEADER_SIZE) == -1)
		error("Error reading file");
	memcpy(filename, header + FILENAME_OFFSET, ENTRY_NAME_SIZE);
	memcpy(file_length, header + FILE_LENGTH_OFFSET, FILE_LENGTH_SIZE);
	free(header);
	header = NULL;
}

// Read data from file and write to output file
void read_file_content(int fd, uint32_t offset, char* filename, uint32_t file_length) {
	int out_fd = open(filename, O_WRONLY | O_CREAT, S_IWUSR | S_IRGRP | S_IRUSR);
	uint32_t blocks = file_length / BLOCK_SIZE;
	uint32_t additional_bytes = file_length % BLOCK_SIZE;
	char* block = (char*) malloc(BLOCK_SIZE);
	lseek(fd, offset, SEEK_SET);
	lseek(out_fd, 0, SEEK_SET);
	for (uint32_t i = 0; i < blocks; i++) {
        // Read BLOCK_SIZE bytes from input file
		if (read(fd, block, BLOCK_SIZE) == -1)
			error("Error reading file");
        // Write BLOCK_SIZE bytes to output file
		if (write(out_fd, block, BLOCK_SIZE) == -1)
			error("Error writing file");
	}

	if (additional_bytes > 0) {
        // Read additional bytes from input file
		if (read(fd, block, additional_bytes) == -1)
			error("Error reading file");
        // Write additional bytes to output file
		if (write(out_fd, block, additional_bytes) == -1)
			error("Error writing file");
	}
	free(block);
	block = NULL;
    // Close output file
    if (close(out_fd) == 1)
        error("Error closing output file");
}

// Entry point
int main(int argc, char* argv[]) {
	if (argc != 2)
		error("Not enough arguments");
	char* filename = argv[1];
    // Check if user uses --help
	if (!strcmp(filename, "--help")) {
		printf("%s\n", help);
		exit(EXIT_SUCCESS);
	}
    // Open input file
	int fd = open(filename, O_RDONLY);
	if (fd == -1)
		error("Error opening file");
	char directory_name[ENTRY_NAME_SIZE];
	uint32_t num_files;
    // Read directory name and number of files from header
	read_header(fd, directory_name, &num_files);
	char dir_name[ENTRY_NAME_SIZE + 3];
	sprintf(dir_name, "./%s", directory_name);
	struct stat st = {0};
    // Create directory
	if (stat(directory_name, &st) == -1)
		mkdir(directory_name, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH);
	uint32_t offset = HEADER_SIZE;
	for (uint32_t i = 0; i < num_files; i++) {
		char filename[ENTRY_NAME_SIZE];
		char path[ENTRY_NAME_SIZE * 2 + 4];
		uint32_t file_length;
        // Read filename and file length from file header
		read_file_header(fd, offset, filename, &file_length);
		sprintf(path, "%s/%s", dir_name, filename);
		offset += FILE_HEADER_SIZE;
        // Read file data and write to output file
		read_file_content(fd, offset, path, file_length);
		offset += file_length;
	}
    // Close file
	if (close(fd) == -1)
		error("Error closing file");
}
