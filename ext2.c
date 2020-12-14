#include <string.h>
#include "disk.c"
#include "ext2.h"

int main(){
}

void init_ext2(){
    char* buf;
    disk_read_block(0,buf);
    disk_read_block(1,buf+DEVICE_BLOCK_SIZE);

    //init sp_block
    if(*((int32_t*)(buf)) == MAGIC_NUM){
        SP_BLOCK.magic_num = *((int32_t*)(buf));
        SP_BLOCK.free_block_count = *((int32_t*)(buf+4));
        SP_BLOCK.free_inode_count = *((int32_t*)(buf+8));
        SP_BLOCK.dir_inode_count = *((int32_t*)(buf+12));
        memcpy(SP_BLOCK.block_map,buf+16,128*4);
        memcpy(SP_BLOCK.inode_map,buf+16+128*4,32*4);

        //init inodes
        char* buf;
        for(int i=0;i<1024*32/DEVICE_BLOCK_SIZE;i++){
            disk_read_block(i+2,buf);
            memcpy(inodes+DEVICE_BLOCK_SIZE*i,buf,DEVICE_BLOCK_SIZE);
        }
    }else{
        SP_BLOCK.magic_num = MAGIC_NUM;
        SP_BLOCK.free_block_count = 4063;
        SP_BLOCK.free_inode_count = 1024;
        SP_BLOCK.dir_inode_count = 0;
        memset(SP_BLOCK.block_map,0,128*4);
        memset(SP_BLOCK.inode_map,0,32*4);
        init_dir_inodes();
    }

    current_dir = read_dir_block(0,0);
    root_dir = read_dir_block(0,0);
}



void init_dir_inodes(){
    set_inode_map(0,1);
    SP_BLOCK.dir_inode_count++;
    inodes[0].size = 999;
    inodes[0].link = 1;
    inodes[0].file_type = DIR;
    inodes[0].block_point[0] = get_block_id();

    struct dir_item item;
    item.inode_id =  get_inode_id();
    memcpy(item.name,"~",2);
    item.type = DIR;
    item.valid = VALID;

    write_dir_block(inodes[0].block_point[0],0,item);
}

void add_item_to_dir_inode(struct dir_item item){
    uint32_t block_offset = SP_BLOCK.dir_inode_count/8;
    uint32_t item_offset = SP_BLOCK.dir_inode_count%8;
    SP_BLOCK.dir_inode_count++;
    memcpy(inodes[0].block_point[block_offset]+item_offset*128,&item,128);
    write_dir_block(block_offset,item_offset,item);
}

struct dir_item find_in_inode(uint32_t inode_to_search,char* name_to_find){
    struct inode node = inodes[inode_to_search];
    for(int i=0;i<node.link;i++){
        struct dir_item temp = read_dir_block(node.block_point[i/8],i%8);
        if(strcmp(name_to_find,temp.name)) return temp;
    }
    struct dir_item temp;
    temp.type = NOT_FOUND;
    return temp;
}
