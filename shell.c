#include "ext2.c"
#include "shell.h"

int main(){
    shell();
}


void shutdown(){
    char* buf = &SP_BLOCK;
    disk_write_block(0,buf);
    disk_write_block(1,buf+512);
}

int touch(char* name,int len){
    if(SP_BLOCK.free_block_count == 0 || SP_BLOCK.free_inode_count == 0 || SP_BLOCK.dir_inode_count == MAX_DIR_NUM){
        printf("error: full");
        return -1;
    }

    struct dir_item item;
    item.inode_id = get_inode_id();
    uint32_t block = get_block_id;
    set_inode_map(item.inode_id,1);
    set_block_map(block,1);
    item.valid = VALID;
    item.type = FILE;
    memcpy(item.name,name,len);
    
    inodes[item.inode_id].size = 0;
    inodes[item.inode_id].file_type = FILE;
    inodes[item.inode_id].link = 1;
    inodes[item.inode_id].block_point[0] = block;

    add_item_to_dir_inode(item);
    return 0;
}

int mkdir(char* name,int len){
    if(SP_BLOCK.free_block_count == 0 || SP_BLOCK.free_inode_count == 0 || SP_BLOCK.dir_inode_count == MAX_DIR_NUM){
        printf("error: full");
        return -1;
    }

    struct dir_item item;
    item.inode_id = get_inode_id();
    uint32_t block = get_block_id();
    set_inode_map(item.inode_id,1);
    set_block_map(block,1);
    item.valid = VALID; 
    item.type = DIR;
    memcpy(item.name,".",2);
    
    inodes[item.inode_id].size = 0;
    inodes[item.inode_id].file_type = DIR;
    inodes[item.inode_id].link = 2;

    write_dir_block(block,0,item);
    struct dir_item temp = current_dir;
    memcpy(temp.name,"..",3);
    write_dir_block(block,1,temp);    

    add_item_to_dir_inode(item);
    add_file(inodes[current_dir.inode_id],item);
    return 0;
}

void ls(){
    struct dir_item dir_item_pointers[64];
    uint32_t size = inodes[current_dir.inode_id].size;
    int i=0,j=0;
    while(j*8+i<size){
        dir_item_pointers[i+j*8] = read_dir_block(inodes[current_dir.inode_id].block_point[j],i++);
        if(i==8) i=0;
    }
    for(int i=0;i<size;i++){
        printf("%s\n",dir_item_pointers[i].name);
    }
}

int cp(char* name1,char* name2,int len1,int len2){
    struct dir_item src_dir_item,dst_dir_item;
    struct dir_item now_dir;
    if(name1[0]=='~'){
        now_dir = root_dir;
    }else{
        now_dir = current_dir;
    }
    char* dirs[] = split(name1,len1);
    int i=0;
    while(dirs[i]!="\0"){
        now_dir = find_in_inode(now_dir.inode_id,dirs[i]);
    }
    
}

void shell(){
    //TODO
}