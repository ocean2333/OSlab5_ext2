#include "ext2.c"
#include "shell.h"

int main()
{
    shell();
    return 0;
}

void shutdown()
{
    char *buf = (char *)&SP_BLOCK;
    disk_write_block(0, buf);
    disk_write_block(1, buf + 512);
    for(int i=0;i<1024*sizeof(struct inode)/DEVICE_BLOCK_SIZE;i++){
        disk_write_block(2+i,(char*)inodes+i*DEVICE_BLOCK_SIZE);
    }
    close_disk();
}

int touch(char *path, int len)
{
    printf("start touch\n");
    struct dir_item item,par_dir,temp;
    char name[50];
    if (SP_BLOCK.free_block_count == 0 || SP_BLOCK.free_inode_count == 0 || SP_BLOCK.dir_inode_count == MAX_DIR_NUM)
    {
        printf("error: full\n");
        return -1;
    }
    if(find_path_to_place_file(path,&par_dir)!=0){
        printf("error: path error\n");
        return -1;
    }
    if(find_name(path,name)!=0){
        printf("error: name error\n");
        return -1;
    }
    if(find_in_inode(par_dir.inode_id,name,&temp,FILE)==0){
        printf("error: already have same name file\n");
        return -1;
    }
    item.inode_id = get_inode_id();
    set_inode_map(item.inode_id, 1);
    printf("created item %s,inode id:%d\n",name,item.inode_id);
    item.valid = VALID;
    item.type = FILE;
    strcpy(item.name, name);

    inodes[item.inode_id].size = 0;
    inodes[item.inode_id].file_type = FILE;
    inodes[item.inode_id].link = 0;

    add_file(par_dir.inode_id,item);

    return 0;
}

int mkdir(char *path, int len)
{
    struct dir_item item,par_dir,tem;
    char name[50];
    if (SP_BLOCK.free_block_count == 0 || SP_BLOCK.free_inode_count == 0 || SP_BLOCK.dir_inode_count == MAX_DIR_NUM)
    {
        printf("error: full");
        return -1;
    }
    if(find_path_to_place_file(path,&par_dir)!=0){
        return -1;
    }
    if(find_name(path,name)!=0){
        return -1;
    }
    if(find_in_inode(par_dir.inode_id,name,&tem,DIR)==0){
        printf("error: already have same name dir\n");
        return -1;
    }
    item.inode_id = get_inode_id();
    uint32_t block = get_block_id();
    set_inode_map(item.inode_id, 1);
    printf("created item %s,inode id:%d\n",name,item.inode_id);

    set_block_map(block, 1);
    printf("this dir takes block id:%d\n",block);
    item.valid = VALID;
    item.type = DIR;
    strcpy(item.name, ".");

    inodes[item.inode_id].size = 0;
    inodes[item.inode_id].file_type = DIR;
    inodes[item.inode_id].link = 2;
    inodes[item.inode_id].block_point[0] = block;


    write_dir_block(BLOCK_ID_OFFSET+block, 0, item);
    struct dir_item temp = current_dir;
    strcpy(temp.name, "..");
    write_dir_block(BLOCK_ID_OFFSET+block, 1, temp);

    strcpy(item.name,name);
    add_file(current_dir.inode_id, item);
    return 0;
}

int ls()
{
    struct dir_item dir_item_pointers[64];
    uint32_t size = inodes[current_dir.inode_id].link;
    int i = 0, j = 0;
    while (j * 8 + i < size)
    {
        dir_item_pointers[i + j * 8] = read_dir_block(BLOCK_ID_OFFSET+inodes[current_dir.inode_id].block_point[j], i);
        i++;
        if (i == 8){
            i = 0;
            j++;
        }    
    }
    for (int i = 0; i < size; i++)
    {
        if(dir_item_pointers[i].type==DIR){
            printf("dir: %s\n", dir_item_pointers[i].name);
        }
        if(dir_item_pointers[i].type==FILE){
            printf("file: %s\n", dir_item_pointers[i].name);
        }
    }
    return 0;
}

int cp(char *src_name, char *dst_name, int len1, int len2)
{
    struct dir_item src_dir_item,dst_dir_item;
    struct dir_item now_dir,next_dir;
    char name[50];
    //find first file
    // if (src_name[0] == '/')
    // {
    //     now_dir = root_dir;
    //     src_name += 1;
    // }
    // else
    // {
    //     now_dir = current_dir;
    // }
    char *token = "/";
    // char *p;
    // strtok(src_name, token);
    // while (p = strtok(NULL, token))
    // {
    //     find_in_inode(now_dir.inode_id, p, &next_dir);
    //     if(!(&next_dir)){
    //         printf("error:path wrong\n");
    //         return -1;
    //     }
    //     now_dir = next_dir;
    // }
    // if (now_dir.type == DIR)
    // {
    //     printf("error:%s not a file\n", src_name);
    //     return -1;
    // }
    // src_dir_item = now_dir;
    if(find_file(src_name,&src_dir_item)!=0) return -1;
    //find second file
    // if (dst_name[0] == '/')
    // {
    //     now_dir = root_dir;
    //     dst_name += 1;
    // }
    // else
    // {
    //     now_dir = current_dir;
    // }
    // strtok(dst_name, token);
    // while (p = strtok(NULL, token))
    // {
    //     find_in_inode(now_dir.inode_id, p, &next_dir);
    //     if(!(&next_dir)&&strtok(NULL, token)){
    //         printf("error:path wrong\n");
    //         return -1;
    //     }
    //     if(!(&next_dir))now_dir = next_dir;
    // }
    if(find_path_to_place_file(dst_name,&dst_dir_item)!=0) return -1;
    if(find_name(dst_name,name)!=0) return -1;
    if(inodes[dst_dir_item.inode_id].link == MAX_FILE_NUM){
        printf("error:dir full\n");
    }else{    
        struct dir_item new_cpy = src_dir_item;
        //copy inode
        uint32_t inode_p = get_inode_id();
        printf("inode id %d is a copy of inode id %d\n",inode_p,src_dir_item.inode_id);
        new_cpy.inode_id = inode_p;
        set_inode_map(inode_p,1);
        inodes[inode_p].file_type = inodes[src_dir_item.inode_id].file_type;
        inodes[inode_p].link = inodes[src_dir_item.inode_id].link;
        inodes[inode_p].size = inodes[src_dir_item.inode_id].size;
        //copy block
        uint16_t temp = inodes[inode_p].link;
        for(int i=0;i<temp;i++){
            inodes[inode_p].block_point[i] = get_block_id();
            set_block_map(inodes[inode_p].block_point[i],1);
            char *buf;
            buf = malloc(DATA_BLOCK_SIZE);
            read_block(BLOCK_ID_OFFSET + inodes[src_dir_item.inode_id].block_point[i],buf);
            write_block(BLOCK_ID_OFFSET + inodes[inode_p].block_point[i],buf);
        }

        strcpy(new_cpy.name,name);
        add_file(dst_dir_item.inode_id,new_cpy);
    }
    return 0;
}

int cd(char* path,int len){
    struct dir_item cur_dir;
    //if do not contain '/',don't need find
    if(find_dir(path,&cur_dir)!=0){
        printf("error:can not find path\n");
        return -1;
    }
    printf("change dir from %s(inode id:%d) to %s(inode id:%d)\n",current_dir.name,current_dir.inode_id,cur_dir.name,cur_dir.inode_id);
    current_dir = cur_dir;
    return 0;
}

int rm(char* path,int len){

}

void shell()
{
    init_ext2();
    char* cmd[5];
    char *arg1,*arg2;
    arg1 = malloc(100);
    arg2 = malloc(100);
    size_t size = 100;
    printf("! ");
    while (scanf("%s",arg1))
    {   
        // for(int i=0;i<1;i++){
        //     printf("%s",cmd[i]);
        //     printf("%d",strcmp("",cmd[i]));
        //     if(!strcmp("",cmd[i])) break;
        // }
        
        if (!strcmp(arg1, "touch"))
        {   
            printf("please enter file name to touch\n");
            scanf("%s",arg1);
            if(touch(arg1, strlen(arg1))!=0){
                printf("error in touch\n");
            }
        }
        else if (!strcmp(arg1, "mkdir"))
        {
            printf("please enter dir name to make\n");
            scanf("%s",arg1);
            if(mkdir(arg1, strlen(arg1))!=0){
                printf("error in mkdir\n");
            }
        }
        else if (!strcmp(arg1, "ls"))
        {
            printf("showing:\n");
            if(ls()!=0){
                printf("error in ls\n");
            }
        }
        else if (!strcmp(arg1, "shutdown"))
        {
            shutdown();
            return ;
        } 
        else if (!strcmp(arg1, "cp"))
        {   
            printf("please enter file to copy\n");
            //getline(cmd, &size, stdin);
            scanf("%s",arg1);
            printf("please place to paste\n");
            scanf("%s",arg2);
            //getline(&cmd[1], &size, stdin);     
            if(cp(arg1, arg2, strlen(arg1), strlen(arg2))!=0){
                printf("error in cp\n");
            }
        }
        else if(!strcmp(arg1, "cd"))
        {
            printf("please enter dir to change\n");
            scanf("%s",arg1);
            if(cd(arg1,strlen(arg1))!=0){
                printf("error in cd\n");
            }
        }
        printf("! ");
    }
}