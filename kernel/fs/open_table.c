//This file contains the table of open files, implemented as an array
// with methods like add_to_opentable and delete_from_opentable to be called in open and close 
//Since methods are provided, other files should not touch neither the free LL or the arrray, 
// but just use the premade methods. 

#include "../include/file.h"
#include "../include/klibc.h"
#include "../include/open_table.h"
#include "../include/bitvector.h"
#include <stdint.h>


//called by file.c initialization function, initializes free list and table
void fs_table_init() {
        free_list = makeVector(SYSTEM_SIZE); //create bitvector of free indexes
        table = kmalloc(SYSTEM_SIZE * (sizeof(struct file_descriptor*))); //malloc the table
}

//at shutdown, memory with the free list is freed
void fs_table_shutdown() {
        bv_free(free_list);
        int i;
        for (i = 0; i < SYSTEM_SIZE; i++) {
                if (table[i] != 0x0) { //deal with users that forget to close files
                        if (table[i]->linked_file != 0x0) { //"if" added because we never know what can happen...
                                kfree(table[i]->linked_file);
                        }
                        kfree(table[i]);
                }
        }
        kfree(table);
}


//returns struct of descriptor at index fd
//if invalid, returns NULL.
struct file_descriptor* get_descriptor(int fd){
	if (file_is_open(fd))
		return table[fd];
	return 0x0;
}

// this function can be used to insert a file in the table
// and returns the requested index if successful, else -1 
int add_to_opentable(struct inode * f, char perm) {
        int fd = (int) bv_firstFree(free_list); //gets free index from bitvector
        if (fd == -1) {
                return -1; //reached max num of files open
        }
        bv_set((uint32_t)fd, free_list); //index is now taken
        struct file_descriptor* to_add = (struct file_descriptor*) kmalloc(sizeof(struct file_descriptor)); //malloc new struct
        int inum = f->inum;
        int i;
        for (i=0; i<SYSTEM_SIZE; i++) {  //checks if file is already open in the table. If so, the linked file is the same. 
                if (table[i]->linked_file->inum == inum) {
                        to_add->linked_file->fd_refs++; //increment the number of references
                        to_add->linked_file =  table[i]->linked_file  //point to same file
                        to_add->permission = perm; //assign new permission
                        to_add->offset = 0; //restart offset from 0
                        table[fd] = to_add; //add to table
                        return fd;
                }
        }
        to_add->linked_file = f;
        to_add->linked_file->fd_refs = 1;
	to_add->permission = perm;
        to_add->offset = 0;
        table[fd] = to_add; //add to table
        if (perm == 'a') { //append, need to move offset to very end
                table[fd]->offset = table[fd]->linked_file->size;
        }
        return fd;
}


//this function can be used to delete a file from the list
//returns 0 if all ok, -1 if wrong
int delete_from_opentable(int fd) {
        if (!file_is_open(fd)) {
                return -1; //invalid entry
        }
        table[fd]->linked_file->fd_refs--;
        if (table[fd]->linked_file->fd_refs == 0) { //free inode only if was referenced only once
                kfree(table[fd]->linked_file);
        }
        kfree(table[fd]); //free space in table
        bv_lower ((uint32_t)fd, free_list); //index is not taken anymore
        return 0;
}


//this function checks whether the file is open or not
int file_is_open(int fd) {
	if (fd<0 || fd>=SYSTEM_SIZE) { 
                return 0; 
        }
        return (table[fd] != 0x0);
}

