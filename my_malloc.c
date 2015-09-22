#include "my_malloc.h"

//Implementation By: Sneh Munshi

/* You *MUST* use this macro when calling my_sbrk to allocate the 
 * appropriate size. Failure to do so may result in an incorrect
 * grading!
 */
#define SBRK_SIZE 2048

/* If you want to use debugging printouts, it is HIGHLY recommended
 * to use this macro or something similar. If you produce output from
 * your code then you will receive a 20 point deduction. You have been
 * warned.
 */
#ifdef DEBUG
#define DEBUG_PRINT(x) printf x
#else
#define DEBUG_PRINT(x)
#endif


/* make sure this always points to the beginning of your current
 * heap space! if it does not, then the grader will not be able
 * to run correctly and you will receive 0 credit. remember that
 * only the _first_ call to my_malloc() returns the beginning of
 * the heap. sequential calls will return a pointer to the newly
 * added space!
 * Technically this should be declared static because we do not
 * want any program outside of this file to be able to access it
 * however, DO NOT CHANGE the way this variable is declared or
 * it will break the autograder.
 */
void* heap;

/* our freelist structure - this is where the current freelist of
 * blocks will be maintained. failure to maintain the list inside
 * of this structure will result in no credit, as the grader will
 * expect it to be maintained here.
 * Technically this should be declared static for the same reasons
 * as above, but DO NOT CHANGE the way this structure is declared
 * or it will break the autograder.
 */
metadata_t* freelist[8];
/**** SIZES FOR THE FREE LIST ****
 * freelist[0] -> 16
 * freelist[1] -> 32
 * freelist[2] -> 64
 * freelist[3] -> 128
 * freelist[4] -> 256
 * freelist[5] -> 512
 * freelist[6] -> 1024
 * freelist[7] -> 2048
 */

//Method Headers for all the helper methods
int log_Base_2(int num);
metadata_t* getBuddy (metadata_t* memBlock);
void remove_freelist_top(metadata_t* memBlock, int freelist_index);
void remove_from_freelist(metadata_t* memBlock);
void add_freelist_top(metadata_t* memBlock, int freelist_index);
void split_memBlock(metadata_t* memBlock, int freelist_index);
void merge_memBlock(metadata_t* memBlock);

#define FREELIST_SIZE 8 

/*
 * This is a helper method to find the log base 2 of a number
 *
 * Please note that this method works correctly only for powers of two
 *
 * It is specifically being used in the context of this code for malloc
 * because as a programmer I am only applying it to powers of two
 * DO NOT TRY TO USE IT AS A GENERAL METHOD FOR FINDING ANY LOG
 */
int log_Base_2(int num)
{
	int count = 0;
	//Since the powers of two look like 10, 100, 1000 etc.
	//Keep shifting till you end up shifting the only one in the binary representation
	while(num >> (count + 1))
	{
		count = count + 1;
	}
	return count;
}

/*
 * This is a helper method to get the buddy of a given block based on it's size and address
 * This method follows the algorithm given in the homework pdf
 */
metadata_t* getBuddy (metadata_t* memBlock)
{
	int i = log_Base_2(memBlock->size);
	//Finding the log base 2 of a block of memory based on its size

	int mask = (int)(0x1 << i);
	//The address of the buddy has almost the same address as that of the block except
	//for the ith bit which is the opposite for the two buddies

	int buddyPointer = (int)(((char*) memBlock) - (char*)heap);
	//subtracting the heap pointer from the block's address to make it start from 0

	return (metadata_t*) ((buddyPointer ^ mask) + (char*)heap);
	//returning the pointer to the buddy by masking the address and adding the heap again to bring 
	//it back to where it was supposed to be

}

/*
 * This is a helper method used to remove a given block from the front of the freelist at a 
 * given index and the index indicates the size of the blocks that we are working with
 */
void remove_freelist_top(metadata_t* memBlock, int freelist_index)
{
	freelist[freelist_index] = memBlock->next;
	//Since I want to remove the memBlock from the free list I am making the 
	//freelist pointer point to the next block that comes after the block to
	//be removed

	if(memBlock->next)
	{
		memBlock->next->prev = NULL;
		memBlock->next = NULL;
		//If the block to be removed has an actual block after it, I want to set the
		//pointers linking them to NULL
	}
}

/*
 * This is a helper method to remove a block from the free list
 * This method would be helpful when I want to merge two buddies into a bigger block
 * In that case I would need to remove it from the existing index and it could be present anywhere
 * in the middle of the list, once I remove it from the existing index, I can merge it with it's buddy
 * to make a bigger block and add that to the higher index in the list
 */
void remove_from_freelist(metadata_t* memBlock)
{
	//if the block has a previous set the links correctly
	if(memBlock->prev)
	{
		memBlock->prev->next = memBlock->next;
	}

	//if the block has a next set the links correctly
	if(memBlock->next)
	{
		memBlock->next->prev = memBlock->prev;
	}

	//finally to remove the block completely set it's linking connecting it to the old
	//blocks by setting them to NULL
	memBlock->prev = NULL;
	memBlock->next = NULL;
}

/*
 * This is a helper method that adds a block of the memory at the appropriate location based 
 * on the index of the freelist to the front of the freelist
 */
void add_freelist_top(metadata_t* memBlock, int freelist_index)
{
	//if the freelist's index pointer is not pointing to any block, then memBlock if the 
	//first block to be added
	if(freelist[freelist_index] == NULL)
	{
		freelist[freelist_index] = memBlock;
		memBlock->next = NULL;
		memBlock->prev = NULL;
	} //otherwise we add memBlock first setting up the links correctly, then we set the index pointer
	else
	{
		memBlock->next = freelist[freelist_index];
		freelist[freelist_index]->prev = memBlock;
		memBlock->prev = NULL;
		freelist[freelist_index] = memBlock;
	}
}

/*
 * This is a helper method used to split a block of memory into two equal halves
 * This is helpful when we are splitting a bigger block to give smaller blocks until
 * we find the best fit for the required block asked by the user
 */
void split_memBlock(metadata_t* memBlock, int freelist_index)
{
	int new_size = memBlock->size/2;
	//since we are splitting the block into two halves, the new size would be half

	metadata_t *firstBlock, *secondBlock;
	//declaring the two block pointers which will point to the two halves of the block to be split

	firstBlock = memBlock;
	//the first block will point to the original block's address

	secondBlock = (metadata_t*)(((char*) firstBlock) + new_size);
	//the second block would point to the address in the middle of the big block to be split

	freelist[freelist_index] = memBlock->next;
	//we take the one we are splitting out of the list's that specific index
	//since we are removing memBlock from the existing index of freelist

	if(freelist[freelist_index] != NULL)
	{
		freelist[freelist_index]->prev = NULL;
		//if the block after the removed one was an actual block then break it's link with the block to be removed
	}

	//connecting the two split blocks together
	firstBlock->next = secondBlock;
	secondBlock->prev = firstBlock;
	firstBlock->prev = NULL;
	secondBlock->next = NULL;

	//setting their size and in use correctly
	firstBlock->size = new_size;
	secondBlock->size = new_size;
	firstBlock->in_use = 0;
	secondBlock->in_use = 0;

	//finally moving the two connected blocks in the next lower index 
	freelist[freelist_index - 1] = firstBlock;
}

/*
 * This is a helper method used to merge a block with it's buddy if both the blocks are
 * free to be merged into one
 */
void merge_memBlock(metadata_t* memBlock)
{
	int freelist_index = log_Base_2(memBlock->size) - 4;
	//We are subtracting 4 because the smallest memory block size we have is 16
	//which should correspond to index 0, so log 16 = 4 - 4 would give us 0

	metadata_t* buddy = getBuddy(memBlock);

	//first defining the cases where we cannot merge
	if(memBlock->size >= SBRK_SIZE)
	{
		//if the block has a size more than or equal to 2048
		//then we cannot merge it into anything bigger so we just place it along with
		//2048 blocks and do not merge anything
		add_freelist_top(memBlock, 7);
		return;
	}
	else if (buddy->in_use || (buddy->size != memBlock->size))
	{
		//If the block's buddy is in use or if the block's size does not match with the buddy's size then
		//we do not merge them instead we just place the block at the correct index and return
		add_freelist_top(memBlock, freelist_index);
		return;
	}

	//Removing the ones we are merging from their original position in the free list
	while (buddy == freelist[freelist_index] || memBlock == freelist[freelist_index])
	{
		if(buddy == freelist[freelist_index])
		{
			remove_freelist_top(buddy, freelist_index);
			//buddy's next becomes NULL
		}
		if(memBlock == freelist[freelist_index])
		{
			remove_freelist_top(memBlock, freelist_index);
			//block's next becomes NULL
		}
	}

	//removing the block and buddy from their position in the linked list
	remove_from_freelist(memBlock);
	remove_from_freelist(buddy);

	if(memBlock > buddy)
	{
		buddy->size = buddy->size*2;
		merge_memBlock(buddy);
	}
	else
	{
		memBlock->size = memBlock->size*2;
		merge_memBlock(memBlock);
	}
}

void* my_malloc(size_t size)
{
	//Checking for correct parameter
	int checkSize = size;
	if (checkSize <= 0)
	{
		return NULL;
	}

	int currentSize = 16;
	//We start with the smallest size and move up until we find the first block size that 
	//fits our requirement

	int index = 0;
	//Also keep track of an index which tells us which index of the free list corresponds to our block size

	int adjustedSize = size + sizeof(metadata_t);
	//We need to add the size of the metadata to the size required by the user to account for the overhead

	//Checking for error conditions
	if (size <= 0)
	{
		return NULL;
	}

	if (adjustedSize > SBRK_SIZE)
	{
		ERRNO = SINGLE_REQUEST_TOO_LARGE;
		return NULL;
	}

	while (currentSize < adjustedSize)
	{
		currentSize = currentSize*2;
		index++;
		//keeping doubling the size of the block until I find the first fit and also increment the 
		//index to keep it consistent with the free list
	}

	//if we don't have the block of the required size to give then, start from the index we are at
	//then we keep going down to bigger block sizes until we get a block which is available to be used
	if (freelist[index] == NULL)
	{
		int n = index; 

		while (n < FREELIST_SIZE && freelist[n] == NULL)
		{
			n++;
		}

		if (n >= FREELIST_SIZE)
		{
			metadata_t* brkPtr = my_sbrk(SBRK_SIZE);

			if (brkPtr == NULL)
			{
				//If the call to my_sbrk returns NULL then we have reached the out of memory condition
				ERRNO = OUT_OF_MEMORY;
				return NULL;
			} 

			if (heap == NULL)
			{
				heap = brkPtr;
				//Setting the heap pointer to point to the beginning of the current heap space
				//NOTE THAT ONLY THE FIRST CALL TO MALLOC RETURNS THE BEGINNING OF THE HEAP
			}

			freelist[7] = brkPtr;
			freelist[7]->size = SBRK_SIZE;
			freelist[7]->in_use = 0;
			freelist[7]->next = NULL;
			freelist[7]->prev = NULL;
		}
		else
		{
			split_memBlock(freelist[n], n);
			//We found the block of smallest size that can be used so we split it
			//and we send the split blocks to the index which is one less
		}

		return my_malloc(size);
	}
	else
	{
		//get the first available block from the freelist to give to the user
		metadata_t* give_to_user = freelist[index];

		remove_freelist_top(give_to_user, index);

		give_to_user->in_use = 1;
		give_to_user->next = NULL;
		give_to_user->prev = NULL;
		give_to_user++;
		//skip over the metadata and get that pointer to the free space

		ERRNO = NO_ERROR;
		//return the pointer that comes after skipping over the metadata portion of the block
		return give_to_user;
	}

}


void* my_calloc(size_t num, size_t size)
{
	int total_memBlock = num*size;
	//first calculating the total size of memory needed

	char* memBlock = my_malloc(total_memBlock);
	//now calling malloc using the new total size

	if (memBlock == NULL)
	{
		return NULL;
	}
	for (int i = 0; i < total_memBlock; i++)
	{
		memBlock[i] = 0;
		//initializing all the contiguous values in memory to zero
	}

	return memBlock;
}


void my_free(void* ptr)
{
	if (ptr == NULL)
	{
		return;
	}

	metadata_t* effectivePtr = ((metadata_t*) ptr) - 1;
	//moving the pointer back to the start of the block
	//removing the overhead which was initially added

	if (!effectivePtr->in_use)
	{
		return;
		//if the effective pointer is not being used then it is already free so no need to do anything
	}

	effectivePtr->in_use = 0;
	effectivePtr->next = NULL;
	effectivePtr->prev = NULL;
	ERRNO = NO_ERROR;
	merge_memBlock(effectivePtr);
	//After freeing the block we try to merge it with the buddy in case the buddy is not in use and 
	//the buddy has the same size as the block which was freed
}

void* my_memmove(void* dest, const void* src, size_t num_bytes)
{
	//To account for the overlap condition, we first check which one comes first
	//If the destination comes before the source, then we copy from the other end
	//If the source comes first, then we copy normally
	if ((unsigned long int) dest > (unsigned long int) src)
	{
		for (int i = (num_bytes - 1); i >= 0; i--)
		{
			*(((char*) dest) + i) = *(((char*) src) + i);
		}
	}
	else
	{
		for (int i = 0; i < num_bytes; i++)
		{
			*(((char*) dest) + i) = *(((char*) src) + i);
		}
	}
	return dest;
}
