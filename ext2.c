#include <string.h>
#include <stdlib.h>
#include "disk.c"
#include "ext2.h"

void init_ext2(){
    open_disk();
    printf("loading ext2\n");
    char* buf;
    buf = (char*)malloc(2*DEVICE_BLOCK_SIZE);
    sp_block temp;

    disk_read_block(0,buf);
    disk_read_block(1,(buf+DEVICE_BLOCK_SIZE));
    memcpy(&temp,buf,sizeof(sp_block));
    //init sp_block
    printf("temp.magic_num:%d\n",temp.magic_num);
    if(temp.magic_num == MAGIC_NUM){
        //not first time
        printf("loading data\n");
        SP_BLOCK = temp;
        //memcpy(&SP_BLOCK,&temp,sizeof(sp_block));
        // SP_BLOCK.magic_num = *((int32_t*)(buf));
        // SP_BLOCK.free_block_count = *((int32_t*)(buf+4));
        // SP_BLOCK.free_inode_count = *((int32_t*)(buf+8));
        // SP_BLOCK.dir_inode_count = *((int32_t*)(buf+12));
        // memcpy(SP_BLOCK.block_map,buf+16,128*4);
        // memcpy(SP_BLOCK.inode_map,buf+16+128*4,32*4);

        // init inodes
        char* buff = (char*)inodes;
        for(int i=0;i<1024*32/DEVICE_BLOCK_SIZE;i++){
            disk_read_block(i+2,(char*)buf);
            memcpy(buff+DEVICE_BLOCK_SIZE*i,buf,DEVICE_BLOCK_SIZE);
            //printf("%d\n",i);
        }

    }else{
        //first time
        printf("initing data\n");

        SP_BLOCK.magic_num = MAGIC_NUM;
        SP_BLOCK.free_block_count = 4063;
        SP_BLOCK.free_inode_count = 1024;
        SP_BLOCK.dir_inode_count = 0;
        memset(SP_BLOCK.block_map,0,128*4);
        memset(SP_BLOCK.inode_map,0,32*4);

        init_dir_inodes();

    }

    current_dir = read_dir_block(BLOCK_ID_OFFSET+0,0);
    root_dir = read_dir_block(BLOCK_ID_OFFSET+0,0);
    printf("loading end\n");
    return ;
}

void init_dir_inodes(){
    set_inode_map(0,1);
    //SP_BLOCK.dir_inode_count++;
    inodes[0].size = 0;
    inodes[0].link = 1;
    inodes[0].file_type = DIR;
    inodes[0].block_point[0] = get_block_id();
    set_block_map(inodes[0].block_point[0],1);
    struct dir_item item;
    item.inode_id = 0;
    strcpy(item.name,".");
    item.type = DIR;
    item.valid = VALID;

    write_dir_block(BLOCK_ID_OFFSET + inodes[0].block_point[0],0,item);
}
// void add_item_to_dir_inode(struct dir_item item){
//     //qiyong
//     uint32_t block_offset = SP_BLOCK.dir_inode_count/8;
//     uint32_t item_offset = SP_BLOCK.dir_inode_count%8;
//     SP_BLOCK.dir_inode_count++;
//     memcpy(inodes[0].block_point[block_offset]+item_offset*128,&item,128);
//     write_dir_block(block_offset,item_offset,item);
// }
void write_dir_block(uint32_t block_offset,uint32_t item_offset,struct dir_item item){
    char* buf;
    buf = (char*)malloc(DATA_BLOCK_SIZE);
    read_block(block_offset,buf);
    memcpy(buf+(item_offset*sizeof(struct dir_item)),&item,sizeof(struct dir_item));
    write_block(block_offset,buf);
}
struct dir_item read_dir_block(uint32_t block_offset,uint32_t item_offset){
    char* buf;
    buf = (char*)malloc(DATA_BLOCK_SIZE);
    struct dir_item item;
    read_block(block_offset,buf);
    memcpy(&item,buf+(item_offset*sizeof(struct dir_item)),sizeof(struct dir_item));
    return item;
}

void read_block(uint32_t data_block_offset,char* buf){
    disk_read_block(data_block_offset*2,buf);
    disk_read_block(data_block_offset*2+1,buf+DEVICE_BLOCK_SIZE);
}
void write_block(uint32_t data_block_offset,char* buf){
    disk_write_block(data_block_offset*2,buf);
    disk_write_block(data_block_offset*2+1,buf+DEVICE_BLOCK_SIZE);
}
int add_file(uint32_t dst_inode,struct dir_item src_item){
    int link_num = (inodes[dst_inode].link)++;   
    int bp_offset = link_num/8;
    int item_offset = link_num%8;
    if(link_num%8==0) {
        inodes[dst_inode].block_point[bp_offset] = get_block_id();
        set_block_map(inodes[dst_inode].block_point[bp_offset],1);
    }
    write_dir_block(BLOCK_ID_OFFSET+inodes[dst_inode].block_point[bp_offset],item_offset,src_item);
    printf("item %s is add to dir that inode is %d\n",src_item.name,dst_inode);
    return 0;
}
int find_in_inode(uint32_t inode_to_search,char* name_to_find,struct dir_item* item,uint8_t item_type){
    int count = 0;
    int link_num = inodes[inode_to_search].link;
    uint32_t block_point[6];
    memcpy(block_point,inodes[inode_to_search].block_point,6*sizeof(uint32_t));
    while(count<link_num){
        struct dir_item temp = read_dir_block(BLOCK_ID_OFFSET+block_point[count/8],count%8);
        if(!strcmp(temp.name,name_to_find)) {
            if(temp.type==item_type){
                memcpy(item,&temp,sizeof(struct dir_item)); 
                return 0;
            }
        }
        count++;
    }
    item = NULL;
    return -1;
}

// struct dir_item find_in_inode(uint32_t inode_to_search,char* name_to_find){
//     struct inode node = inodes[inode_to_search];
//     for(int i=0;i<node.link;i++){
//         struct dir_item temp = read_dir_block(node.block_point[i/8],i%8);
//         if(strcmp(name_to_find,temp.name)) return temp;
//     }
//     struct dir_item temp;
//     temp.type = NOT_FOUND;
//     return temp;
// }

int find_file(char* path,struct dir_item* file){
    struct dir_item now_dir,next_dir;
    if (path[0] == '/')
    {
        now_dir = root_dir;
        path += 1;
    }
    else
    {
        now_dir = current_dir;
    }
    char *token = "/";
    char *p;
    p= strtok(path, token);
    while (p!=NULL)
    {
        find_in_inode(now_dir.inode_id, p, &next_dir,FILE);
        if(!(&next_dir)){
            printf("error:path wrong\n");
            return -1;
        }
        now_dir = next_dir;
        p = strtok(NULL, token); 

    }
    if (now_dir.type == DIR)
    {
        printf("error:%s not a file\n", path);
        return -1;
    }
    *file = now_dir;
    return 0;
}

int find_dir(char* path,struct dir_item* dir){
    struct dir_item now_dir,next_dir;
    if (path[0] == '/')
    {
        now_dir = root_dir;
        path += 1;
    }
    else
    {
        now_dir = current_dir;
    }
    char *token = "/";
    char *p;
    p= strtok(path, token);
    while (p!=NULL)
    {
        find_in_inode(now_dir.inode_id, p, &next_dir,DIR);
        if(!(&next_dir)){
            printf("error:path wrong\n");
            return -1;
        }
        now_dir = next_dir;
        p = strtok(NULL, token); 

    }
    if (now_dir.type == FILE)
    {
        printf("error:%s not a file\n", path);
        return -1;
    }
    *dir = now_dir;
    return 0;
}

int find_path_to_place_file(char* dst_name,struct dir_item* dir){
    char temp[50];
    strcpy(temp,dst_name);
    //if do not contain '/',don't need find
    if(strstr(dst_name,"/")==NULL){
        *dir = current_dir;
        return 0;
    }
    //remove file name
    for(int i=strlen(dst_name)-1;i>=0;i--){
        char c = dst_name[i];
        dst_name[i] = '\0';
        if(c=='/') break;
    }
    struct dir_item now_dir,next_dir;
    if (dst_name[0] == '/')
    {
        now_dir = root_dir;
        dst_name += 1;
    }
    else
    {
        now_dir = current_dir;
    }
    char *token = "/";
    char *p;
    p=strtok(dst_name, token);
    while (p!=NULL)
    {
        int res = find_in_inode(now_dir.inode_id, p, &next_dir,DIR);
        if(!(&next_dir)&&strtok(NULL, token)){
            printf("error:path wrong\n");
            return -1;
        }
        if(&next_dir && res==0)now_dir = next_dir;
        p = strtok(NULL, token);
    }
    *dir = now_dir;
    strcpy(dst_name,temp);
    return 0;
}
int find_name(char* path,char* name){
    //can not end with /
    if(path[strlen(path)-1]=='/') {
        printf("error:wrong path\n");        
        return -1;
    }
    //if do not contain '/',the path is name
    if(strstr(path,"/")==NULL){
        strcpy(name,path);
        return 0;
    }
    char res[50];
    int offset = 0;
    for(int i=0;path[i]!='\0';i++){
        if(path[i]!='/') res[offset++] = path[i];
        else{
            offset=0; 
        }
    }
    res[offset] = '\0';
    strcpy(name,res);
    // char *token = "/";
    // char *p;
    // p=strtok(path, token);
    // while(p=NULL){
    //     strcpy(name,p);
    //     p=strtok(path, token);
    // }
    return 0;
}