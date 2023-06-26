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
		struct map_data* curData = &md[i];

		curData->mapFile = argv[i+1];
		curData->fd = open(curData->mapFile, O_RDONLY);

		printf("Opened %s into fd=%d\n", curData->mapFile, curData->fd);

		curData->fileLen = lseek(curData->fd, 0, SEEK_END);
		printf("  File / map length = %d\n", curData->fileLen);
		
		curData->mapPtr = mmap(0, curData->fileLen, PROT_READ, MAP_PRIVATE, curData->fd, 0);
		printf("  Pointer to mmap at %p\n", curData->mapPtr);

		// You can close a file and keep the map to it open...
		printf("Closing the file\n");
		close(curData->fd);

		curData->sillyChecksum = 0;
		uint8_t* data = curData->mapPtr;
		for(int filePos = 0; filePos < curData->fileLen; filePos++)
		{
			curData->sillyChecksum += data[filePos];
		}

		printf("Checksum = 0x%08x\n", curData->sillyChecksum);

	}

	printf("Sleeping for 5 seconds\n");
	sleep(5);

	for (int i = 0; i < numFiles; i++)
	{
		struct map_data* curData = &md[i];
		printf("Closing the mmap for %s\n", curData->mapFile);
		munmap(curData->mapPtr, curData->fileLen);
	}

	printf("Exitting\n");

	return 0;
}
