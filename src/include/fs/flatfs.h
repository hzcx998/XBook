

/*
FFS，平坦文件系统（Flat File System）

*/

/* 分类名称长度 */
#define FFS_CATEGORY_NAME_LEN   128
/**
 * 种类，每一种类型的文件都有一个分类
 */
struct FFS_Category {
    char name[FFS_CATEGORY_NAME_LEN];

};
