
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include "ext2_fs.h"


#define SUP_BLOCK_OFFSET 1024
#define SUP_BLOCK_LEN 1024
#define GR_DESC_LEN 32
#define GR_DESC_OFFSET 2048 //sb offset + sb len
#define DIR_ENTRY_LEN 263 //8 + 255 (max) for name
#define INODE_SIZE 128
#define USED 1
#define FREE 0

struct ext2_group_desc* group_desc = NULL;
struct ext2_inode* inodes = NULL;
struct ext2_super_block* superblock = NULL;


int fd = -1;
int n_dirs = 0;
unsigned int blocksize = EXT2_MIN_BLOCK_SIZE;

//http://cs.smith.edu/~nhowe/262/oldlabs/ext2.html
#define BASE_OFFSET 1024  /* location of the super-block in the first group */
#define BLOCK_OFFSET(block) (BASE_OFFSET + (block-1)*blocksize)

void errorcheck(int check, char* func, char* routine) {
	if (check < 0) {
		fprintf(stderr, "*** Error in the %s function in the %s routine.\n%s", func, routine, strerror(errno));
		exit(2);
	}
}

// via https://www.epochconverter.com/programming/c


void init(char* fs_image, int num_args) {
	if (num_args == 2) {
		fd = open(fs_image, O_RDONLY);
		//errorcheck(fd, "open", "main");
		if (fd < 0) {
			fprintf(stderr, "*** Error: could not open the image file. Please make sure to use a valid .img file\n%s", strerror(errno));
			exit(1);
		}
	}	
	else {
		fprintf(stderr, "*** Bad arguments! Usage: ./lab3a filesystem.img\n");
		exit(1);
	}

	group_desc = (struct ext2_group_desc*) malloc(sizeof(struct ext2_group_desc));
	superblock = (struct ext2_super_block*) malloc(sizeof(struct ext2_super_block));

	int check1 = pread(fd, group_desc, GR_DESC_LEN, GR_DESC_OFFSET);
	errorcheck(check1, "pread", "init");
	int check2 = pread(fd, superblock, SUP_BLOCK_LEN, SUP_BLOCK_OFFSET);
	errorcheck(check2, "pread", "init");

	return;

}

void superblock_summ(void) {
	blocksize <<= superblock->s_log_block_size;
	printf("SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n", superblock->s_blocks_count, superblock->s_inodes_count, blocksize,
		superblock->s_inode_size, superblock->s_blocks_per_group, superblock->s_inodes_per_group, superblock->s_first_ino);
}

void group_summ(void) {
	//we only have one group, so we print 0
	printf("GROUP,0,%d,%d,%d,%d,%d,%d,%d\n", superblock->s_blocks_count, superblock->s_inodes_count, group_desc->bg_free_blocks_count, group_desc->bg_free_inodes_count,
		group_desc->bg_block_bitmap, group_desc->bg_inode_bitmap, group_desc->bg_inode_table);
}

void free_block(void) {

	int nblocks = superblock->s_blocks_count;
	uint8_t* bitmap = (uint8_t*) malloc(blocksize);
	pread(fd, bitmap, blocksize, BLOCK_OFFSET(group_desc->bg_block_bitmap)); //get the actual bitmap
	int i;
	for (i = 0; i < nblocks; i++) {
		int bcheck = bitmap[i];
		int j; 
		for (j = 0; j < 8; j++) {
			if (!(bcheck & 1)) {
				int curblock = (8 * i) + (j + 1);
				printf("BFREE,%d\n", curblock);
			}
			bcheck >>= 1;
		}
	}
	free(bitmap);
	bitmap = NULL;
}

void free_inode(void) {
	int n_inodes = superblock->s_inodes_count;
	uint8_t* bitmap = (uint8_t*) malloc(blocksize);
	pread(fd, bitmap, blocksize, BLOCK_OFFSET(group_desc->bg_inode_bitmap));
	int i;
	for (i = 0; i < n_inodes; i++) {
		int icheck = bitmap[i];
		int j; 
		for (j = 0; j < 8; j++) {
			if (!(icheck & 1)) {
				int curblock = (8 * i) + (j + 1);
				printf("IFREE,%d\n", curblock);
			}
			icheck >>= 1;
		}
	}
	free(bitmap);
	bitmap = NULL;
}


/*
	indirect 1: blocks 13 to 268
	indirect 2: blocks 269 to 65804
	indirect 3: 65805 to something big

	(BECAUSE BLOCKS START FROM 0)
	for level 1: USE 12
	for level 2: USE 268
	for level 3: USE 65804

	This is the main idea

	*************************************
	*** You should actually be using: ***
	*************************************

	for level 1: USE 12
	for level 2: USE 12 + blocksize/sizeof(uint32_t)
	for level 3: USE 12 + blocksize/sizeof(uint32_t) + (blocksize/sizeof(uint32_t))^2
*/
/*
void indirect_handler(uint32_t owner_inode_num, uint32_t log_block_offset, uint32_t indirect_block_num) {
	uint32_t *block = (uint32_t*) malloc(blocksize);
	pread(fd, block, blocksize, BLOCK_OFFSET(indirect_block_num));
	int n = (int) blocksize / sizeof(uint32_t); //bc we allocated a certain number of bytes for something that isn't one byte per index
	int i;
	for (i = 0; i < n; i++) {
		if (block[i] == 0) {
			continue; 	//because we can have sparse files
		}
		printf("INDIRECT,%d,%d,%d,%d,%d\n", owner_inode_num, 1, i + log_block_offset, indirect_block_num, block[i]); 
		//i + log_block_offset bc as we move to the next entries, we move down the table of references and therefore need to add an extra offset for the logic block
	}
}

void double_indirect(uint32_t owner_inode_num, uint32_t log_block_offset, uint32_t indirect_block_num) {
	uint32_t *block = (uint32_t*) malloc(blocksize);
	pread(fd, block, blocksize, BLOCK_OFFSET(indirect_block_num));
	int n = (int) blocksize / sizeof(uint32_t); //bc we allocated a certain number of bytes for something that isn't one byte per index
	int i;
	for (i = 0; i < n; i++) {
		if (block[i] == 0) {
			continue; 	//because we can have sparse files
		}
		printf("INDIRECT,%d,%d,%d,%d,%d\n", owner_inode_num, 2, 12 + (uint32_t) blocksize/sizeof(uint32_t) + log_block_offset, indirect_block_num, block[i]);

		indirect_handler(owner_inode_num, log_block_offset + 12 + (uint32_t) blocksize/sizeof(uint32_t), block[i]);
	}
}

void triple_indirect(uint32_t owner_inode_num, uint32_t indirect_block_num) {
	uint32_t* block = (uint32_t*) malloc(blocksize);
	pread(fd, block, blocksize, BLOCK_OFFSET(indirect_block_num));
	int n = (int) blocksize / sizeof(uint32_t); //bc we allocated a certain number of bytes for something that isn't one byte per index
	int i;
	for (i = 0; i < n; i++) {
		if (block[i] == 0) {
			continue;
		}
		printf("INDIRECT,%d,%d,%d,%d,%d\n", owner_inode_num, 3, 12 + (uint32_t) blocksize/sizeof(uint32_t) + (uint32_t) pow((uint32_t) blocksize/sizeof(uint32_t), 2), indirect_block_num + i, block[i]);

		double_indirect(owner_inode_num, pow((uint32_t) blocksize/sizeof(uint32_t), 2), block[i]);
	}
}
*/


void indirect(int level, uint32_t owner, uint32_t offset, uint32_t blocknum) {
	uint32_t logical_offset = 0;
	uint32_t send_offset = 0;
	uint32_t *block = (uint32_t*) malloc(blocksize);
	pread(fd, block, blocksize, BLOCK_OFFSET(blocknum));
	int n = (int) blocksize / sizeof(uint32_t); //bc we allocated a certain number of bytes for something that isn't one byte per index
	
	int i;
	for (i = 0; i < n; i++) {
		if (level == 1) {
			logical_offset = i + offset;
		}
		else if (level == 2) {
			logical_offset = 12 + (uint32_t) blocksize/sizeof(uint32_t) + offset;
			send_offset = offset + 12 + (uint32_t) blocksize/sizeof(uint32_t);
		}
		else if (level == 3) {
			logical_offset = 12 + (uint32_t) blocksize/sizeof(uint32_t) + (uint32_t) pow((uint32_t) blocksize/sizeof(uint32_t), 2);
			send_offset = pow((uint32_t) blocksize/sizeof(uint32_t), 2);
			blocknum = blocknum + i;
		}
		else {
			free (block);
			return;
		}
		if (block[i] == 0) {
			continue; 	//because we can have sparse files
		}
		printf("INDIRECT,%d,%d,%d,%d,%d\n", owner, level, logical_offset, blocknum, block[i]);
		indirect(level - 1, owner, send_offset, block[i]);
	}
	return;
}




void indirect_block_ref(struct ext2_inode* node, uint32_t owner_inode_num) {
	/*indirect_handler(owner_inode_num, 12, node->i_block[12]);
	double_indirect(owner_inode_num, 0, node->i_block[13]);			//we pass 0 here as logical offset because we already account for it
	triple_indirect(owner_inode_num, node->i_block[14]);*/
	indirect(1, owner_inode_num, 12, node->i_block[12]);
	indirect(2, owner_inode_num, 0, node->i_block[13]);
	indirect(3, owner_inode_num, 0, node->i_block[14]);

}


void print_directories(int level, int parent_num, void* block) {
	if (level == 0) {
		unsigned int size = 0;
		unsigned int logical_offset = 0;
		struct ext2_dir_entry* dir_entry;
		dir_entry = (struct ext2_dir_entry*) block;
		while (size < blocksize) {
			if (dir_entry->inode == 0) {
				break;
			}
			char file_name[EXT2_NAME_LEN + 1];
			memcpy(file_name, dir_entry->name, dir_entry->name_len);
			file_name[dir_entry->name_len] = '\0';
			printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n", parent_num, logical_offset, dir_entry->inode, dir_entry->rec_len, dir_entry->name_len, file_name);

			logical_offset += dir_entry->rec_len;
			dir_entry = (void*) dir_entry + dir_entry->rec_len;
			size += dir_entry->rec_len;
		}
	}
	else if (level <= 3 && level >= 1) {
		//recursive call
		void* sub_block = (void*) malloc(blocksize);
		uint32_t* next_block_ptr;
		int j;
		for (j = 0; j < blocksize / 4; j++) {
			next_block_ptr = block + (4*j); //4 * j because each value in this block is uint32_t meaning 4 bytes -> each value in this block is the number of another block with values;
			if (*next_block_ptr != 0) {
				pread(fd, sub_block, blocksize, blocksize * (*next_block_ptr));
			}
			print_directories(level - 1, parent_num, sub_block);
		}
	}
	else {
		exit(2); //we shouldn't have this. if we do, then something is wrong
		return;
	}
}


void directory(int parent_num, struct ext2_inode* node) {
	void* block = (void*) malloc(blocksize);
	int i;
	for (i = 0; i < 15; i++) {
		memset(block, 0, blocksize);
		pread(fd, block, blocksize, BLOCK_OFFSET(node->i_block[i]));
		if (i < 12) {
			print_directories(0, parent_num, block);
		}
		else if (i == 12 && node->i_block[i] != 0) {
			print_directories(1, parent_num, block);
		}
		else if (i == 13 && node->i_block[i] != 0) {
			print_directories(2, parent_num, block);
		}
		else if (i == 14 && node->i_block[i] != 0) {
			print_directories(3, parent_num, block);
		}
		else {
			free(block);
			return;
		}
	}
	free(block);
}




//via:    http://cs.smith.edu/~nhowe/262/oldlabs/ext2.html

void dir_entries(int parent_num, struct ext2_inode* parent) {
	void* block = (void*) malloc(blocksize); //char* so we can access each byte of the memory

	int i;
	for (i = 0; i < EXT2_NDIR_BLOCKS; i++) { 
		pread(fd, block, blocksize, BLOCK_OFFSET(parent->i_block[i]));
		unsigned int size = 0;
		unsigned int logical_offset = 0;
		struct ext2_dir_entry* dir_entry;
		dir_entry = (struct ext2_dir_entry*) block; //start at the beginning of each block to look at all directory entries
		while (size < parent->i_size) {
			if (dir_entry->inode == 0) {
				break;
			}
			char file_name[EXT2_NAME_LEN + 1];
			memcpy(file_name, dir_entry->name, dir_entry->name_len);
			file_name[dir_entry->name_len] = '\0';
			printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n", parent_num, logical_offset, dir_entry->inode, dir_entry->rec_len, dir_entry->name_len, file_name);

			logical_offset += dir_entry->rec_len;
			dir_entry = (void*) dir_entry + dir_entry->rec_len;
			size += dir_entry->rec_len;
		}

	}
	free(block);
}







void inode_summ(void) {
	int bitmask = 4095; //111111111111 in decimal -> low order 12 bits
	int n_inodes = superblock->s_inodes_count; //bc we only have one group here
	inodes = (struct ext2_inode*) malloc(sizeof(struct ext2_inode) * n_inodes);

	int i;
	for (i = 0; i < n_inodes; i++) {     // we can just go through all of the inodes because we only have one group here
		struct ext2_inode *node = &inodes[i];
		char filetype = '?';

		// ******** +1 because inodes should come right after the inode table? TODO: why not +1??????
		// OOOHHHHHHHH it's not +1 because the inode table already contains inodes -> its not actually a table (?)
		pread(fd, node, INODE_SIZE, (i*INODE_SIZE) + BLOCK_OFFSET(group_desc->bg_inode_table)); 

		int low12mode = node->i_mode & bitmask;
		if (node->i_mode != 0 && (node->i_links_count) != 0) { //only want to print this if they are used/allocated
			//had some issues with the orderings of these if statements because I had symbolic link first which would lead all regular files to be labeled as symbolic links
			if (node->i_mode & 0x8000) {
				filetype = 'f';
				indirect_block_ref(node, i+1);
			}
			else if (node->i_mode & 0x4000) {
				filetype = 'd';
				directory(i+1, node); //i+1 because inode numbers start from 1
				indirect_block_ref(node, i+1);
			}
			else if (node->i_mode & 0xA000) {
				filetype = 's';
			}
			else {
				filetype = '?';
			}

			char atime[32];
			char ctime[32]; 
			char mtime[32]; 

			time_t a_temp, m_temp, c_temp;
			a_temp = (time_t) node->i_atime;
			c_temp = (time_t) node->i_ctime;
			m_temp = (time_t) node->i_mtime;

			struct tm* a = gmtime(&a_temp);
			struct tm* c = gmtime(&c_temp);
			struct tm* m = gmtime(&m_temp);

			strftime(atime, 31, "%m/%d/%y %H:%M:%S", a);
			strftime(ctime, 31, "%m/%d/%y %H:%M:%S", c);
			strftime(mtime, 31, "%m/%d/%y %H:%M:%S", m);

			printf("INODE,%d,%c,%o,%d,%d,%d,%s,%s,%s,%d,%d", i+1, filetype, low12mode, node->i_uid, node->i_gid, node->i_links_count, ctime, mtime, atime, 
				node->i_size, node->i_blocks); //i+1 here because inodes start at 1
			int j;
			for (j = 0; j < EXT2_N_BLOCKS; j++) {
				printf(",%u", node->i_block[j]);
			}
			printf("\n");

		}
	}
}





int main(int argc, char** argv) {
	init(argv[1], argc);
	superblock_summ();
	group_summ();
	free_block();
	free_inode();
	inode_summ();

	free(group_desc);
	free(superblock);
	free(inodes);
	exit(0);
}