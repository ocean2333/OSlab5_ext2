#include <stdint.h>

#define MAGIC_NUM 37
#define DIR_INODE_NUM 0
#define DIR 0x00
#define FILE 0x01
#define VALID 0x0000
#define INVALID 0x0001
#define MAX_DIR_NUM 48
#define MAX_DATA_BLOCK 4063
#define MAX_INODE = 1024

typedef struct super_block {
    int32_t magic_num;                  // 幻数
    int32_t free_block_count;           // 空闲数据块数
    int32_t free_inode_count;           // 空闲inode数
    int32_t dir_inode_count;            // 目录inode数
    uint32_t block_map[128];            // 数据块占用位图
    uint32_t inode_map[32];             // inode占用位图
} sp_block;//16+4*128+4*32B

struct inode {
    uint32_t size;              // 文件大小
    uint16_t file_type;         // 文件类型（文件/文件夹）
    uint16_t link;              // 连接数
    uint32_t block_point[6];    // 数据块指针
};//32B

struct dir_item {               // 目录项一个更常见的叫法是 dirent(directory entry)
    uint32_t inode_id;          // 当前目录项表示的文件/目录的对应inode
    uint16_t valid;             // 当前目录项是否有效 
    uint8_t type;               // 当前目录项类型（文件/目录）
    char name[121];             // 目录项表示的文件/目录的文件名/目录名
};//128B

sp_block SP_BLOCK;
struct inode inodes[1024];
struct dir_item current_dir;

void shutdown();
int touch(char* name,int len);
int mkdir(char* name,int len);
int cp(char* name1,char* name2,int len1,int len2);
void ls();

//初始化ext2
void init_ext2();
//分配一个inode_id
uint32_t get_inode_id();
//分配一个block_id
uint32_t get_block_id();
//将新建的dir_item添加到dir_inode里管理
void add_item_to_dir_inode(struct dir_item item);
//初始化dir_inodes
void init_dir_inodes();
//将dir_block的修改写回硬盘
void write_dir_block(uint32_t block_offset,uint32_t item_offset,struct dir_item item);
//读取一个dir_block
struct dir_item read_dir_block(uint32_t block_offset,uint32_t item_offset);
//读取一个block
void read_block(uint32_t data_block_offset,char* buf);
//写回一个block
void write_block(uint32_t data_block_offset,char* buf);
//操控bit_map
void set_block_map(uint32_t offset,int value){
    int col = offset/32;
    int row = offset%32;
    value = value << row;
    SP_BLOCK.block_map[col] |= value; 
}
int get_block_map(int offset){
    int col = offset/32;
    int row = offset%32;
    return (SP_BLOCK.block_map[col] >> row)&1; 
}
void set_inode_map(uint32_t offset,int value){
    int col = offset/32;
    int row = offset%32;
    value = value << row;
    SP_BLOCK.inode_map[col] |= value; 
}
int get_inode_map(int offset){
    int col = offset/32;
    int row = offset%32;
    return (SP_BLOCK.inode_map[col] >> row)&1; 
}


