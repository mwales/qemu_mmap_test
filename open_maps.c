#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>

struct map_data
{
	int fd;
	char* mapFile;
	int fileLen;
	int sillyChecksum;
	void* mapPtr;
};

int main(int argc, char** argv)
{
	if (argc <= 1)
	{
		printf("Usage: %s [file1 ...]\n", argv[0]);
		printf("  Opens up all the files with open/mmap, and then computes a checksum of each file\n");
		return 0;
	}

	int numFiles = argc - 1;
	printf("Allocating memory for the map_data structure\n");
	struct map_data* md = (struct map_data*) malloc(sizeof(struct map_data) * numFiles);

	for(int i = 0; i < numFiles; i++)
	{
		md[i].mapFile = argv[i+1];
		md[i].fd = open(md[i].mapFile, O_RDONLY);

		printf("Opened %s into fd=%d\n", md[i].mapFile, md[i].fd);

		md[i].fileLen = lseek(md[i].fd, 0, SEEK_END);
		printf("  File / map length = %d\n", md[i].fileLen);
	}

	for(int i = 0; i < numFiles; i++)
	{
		printf("Opening the map for %s\n", md[i].mapFile);

		md[i].mapPtr = mmap(0, md[i].fileLen, PROT_READ, MAP_PRIVATE, md[i].fd, 0);
		printf("  Pointer to mmap at %p\n", md[i].mapPtr);

		printf("Closing the file\n");
		close(md[i].fd);
	}

	printf("Sleeping for 5 seconds\n");
	sleep(5);

	for(int i = 0; i < numFiles; i++)
	{
		int cs = 0;
		uint8_t* data = md[i].mapPtr;
		for(int filePos = 0; filePos < md[i].fileLen; filePos++)
		{
			cs += data[filePos];
		}

		printf("Checksum = %d\n", cs);
	}

	for (int i = 0; i < numFiles; i++)
	{
		printf("Closing the mmap for %s\n", md[i].mapFile);
		munmap(md[i].mapPtr, md[i].fileLen);
	}

	printf("Exitting\n");

	return 0;
}
