/*
 * file:		kernel/mm/memcache.c
 * auther:		Jason Hu
 * time:		2019/10/1
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <lib/string.h>
#include <lib/math.h>
#include <lib/const.h>


/*
 * cacheSizes - cache大小的描述
 */
PRIVATE struct CacheSize cacheSizes[] = {
	#if PAGE_SIZE == 4096
	{32, NULL},
	#endif
	{64, NULL},
	{128, NULL},
	{256, NULL},
	/* 小于1KB的，就在单个页中进行分割 */
	{512, NULL},
	{1024, NULL},
	{2048, NULL},
	{4096, NULL},
	{8*1024, NULL},
	{16*1024, NULL},
	{32*1024, NULL},
	{64*1024, NULL},
	/* 通常情况下最大支持128KB的内存分配，需要在1MB内完成对象分割 */
	{128*1024, NULL},	
	/* 配置大内存的分配 */
	#ifdef CONFIG_LARGE_ALLOCS
	{256*1024, NULL},
	{512*1024, NULL},
	{1024*1024, NULL},
	{2*1024*1024, NULL},
	{4*1024*1024, NULL},	// 64KB
	#endif
	{0, NULL},		// 用于索引判断结束
};

/*
 * 通过页的大小来选择cache数量
 */
#if PAGE_SIZE == 4096
	#ifdef CONFIG_LARGE_ALLOCS 
		#define MAX_MEM_CACHE_NR 18
	#else
		#define MAX_MEM_CACHE_NR 13
	#endif
#else
	#ifdef CONFIG_LARGE_ALLOCS 
		#define MAX_MEM_CACHE_NR 17
	#else
		#define MAX_MEM_CACHE_NR 12
	#endif
#endif



/* 最开始的groupcache */
struct MemCache memCacheTable[MAX_MEM_CACHE_NR];


PUBLIC void DumpMemCache(struct MemCache *cache)
{
	printk(PART_TIP "----Mem Cache----\n");
	printk(PART_TIP "object size %d count %d\n", cache->objectSize, cache->objectNumber);
	printk(PART_TIP "flags %x name %s\n", cache->flags, cache->name);
	printk(PART_TIP "full %x partial %x free %x\n", cache->fullGroups, cache->partialGroups, cache->freeGroups);
	
}

PUBLIC void DumpMemGroup(struct MemGroup *group)
{
	printk(PART_TIP "----Mem Group----\n");
	printk(PART_TIP "map bits %x len %d\n", group->map.bits, group->map.btmpBytesLen);
	printk(PART_TIP "objects %x flags %x list %x\n", group->objects, group->flags, group->list);
	printk(PART_TIP "using %d free %x\n", group->usingCount, group->freeCount);
	
}


PRIVATE INIT int MemCacheInit(struct MemCache *cache,
	char *name, 
	size_t size, 
	flags_t flags)
{
	if (!size)
		return -1;
	// 初始化链表
	INIT_LIST_HEAD(&cache->fullGroups);
	INIT_LIST_HEAD(&cache->partialGroups);
	INIT_LIST_HEAD(&cache->freeGroups);

	/* 根据size来选择不同的储存方式，以节约内存 */
	if (size < 1024) { // 如果是小于1024，那么就放到单个页中。
		/* 地址和8字节对齐，因为在结构体后面存放位图，位图是8字节为单位的 */
		unsigned int groupSize = ALIGN_WITH(SIZEOF_MEM_GROUP, 8);

		/* 如果是32字节为准，那么，就位图就只需要16字节即可 */
		unsigned int leftSize = PAGE_SIZE - groupSize - 16;

		// 对象数量
		cache->objectNumber = leftSize / size;;
	} else if (size <= 128 * 1024) {  // 如果是小于128kb，就放到1MB以内
		cache->objectNumber = (1 * MB) / size;
	} else if (size <= 4 * 1024 * 1024) { // 如果是小于4MB，就放到4MB以内
		cache->objectNumber = (4 * MB) / size;
	}

	// 对象的大小
	cache->objectSize = size;

	// 设定cache的标志
	cache->flags = flags;
	
	// 设置名字
	memset(cache->name, 0, MEM_CACHE_NAME_LEN);
	strcpy(cache->name, name);

	//DumpMemCache(cache);
	return 0;
}

PRIVATE void *MemCacheAllocPages(unsigned int count)
{
	unsigned int page = AllocPages(count);
	if (!page)
		return NULL;
	
	return Phy2Vir(page);
}

PRIVATE int MemCacheFreePages(void *address)
{
	if (address == NULL)
		return -1;

	unsigned int page = Vir2Phy(address);
	
	if (!page)
		return -1;
	
	FreePages(page);
	return 0;
}

PRIVATE INIT int MemGroupInit(struct MemCache *cache,
	struct MemGroup *group,
	flags_t flags)
{
	// 把group添加到free链表
	ListAdd(&group->list, &cache->freeGroups);

	// 位图位于group结构后面
	unsigned char *map = (unsigned char *)(group+1);

	// 设定位图
	group->map.btmpBytesLen = DIV_ROUND_UP(cache->objectNumber, 8);
	group->map.bits = (unsigned char *)map;	
	
	BitmapInit(&group->map);

	struct MemNode *node; 

	/* 根据缓冲中记录的对象大小进行不同的设定 */
	if (cache->objectSize < 1024) {
		group->objects = map + 16;

		/* 转换成节点，并标记 */
		node = Page2MemNode(Vir2Phy(group));
		CHECK_MEM_NODE(node);
		MEM_NODE_MARK_CHACHE_GROUP(node, cache, group);
	} else {
		unsigned int pages = DIV_ROUND_UP(cache->objectNumber * cache->objectSize, PAGE_SIZE); 
		group->objects = MemCacheAllocPages(pages);
		if (group->objects == NULL) {
			printk(PART_ERROR "alloc page for mem objects failed\n");
			return -1;
		}
		int i;
		for (i = 0; i < pages; i++) {
			node = Page2MemNode(Vir2Phy(group->objects + i * PAGE_SIZE));
			CHECK_MEM_NODE(node);
			MEM_NODE_MARK_CHACHE_GROUP(node, cache, group);
		}
	}

	group->usingCount = 0;
	group->freeCount = cache->objectNumber;
	group->flags =  flags;

	//DumpMemGroup(group);
	return 0;
}
/*
 * groupCreate - 创建一个新的group
 * @cache: group所在的cache
 * @flags:创建的标志
 * 
 * 如果成功返回，失败返回-1
 */
PRIVATE int CreateMemGroup(struct MemCache *cache, flags_t flags)
{
	struct MemGroup *group;

	/* 为内存组分配一个页 */
	group = MemCacheAllocPages(1);
	if (group == NULL) {
		printk(PART_ERROR "alloc page for mem group failed!\n");
		return -1;
	}

	if (MemGroupInit(cache, group, flags)) {
		printk(PART_ERROR "init mem group failed!\n");
		goto ToFreeGroup;
	}
	
	// 创建成功
	return 0;

ToFreeGroup:
	// 释放对象组的页
	MemCacheFreePages(group);
	return -1;
}


PRIVATE int MakeMemCaches()
{
	// 指向cache大小的指针
	struct CacheSize *cacheSize = cacheSizes;

	// memCacheTable
	struct MemCache *memCache = &memCacheTable[0];

	//printk(PART_TIP "memCacheTable addr %x size %d\n", memCache, sizeof(struct MemCache));

	// 如果没有遇到大小为0的cache，就会把cache初始化
	while (cacheSize->cacheSize) {
		/* 初始化缓存信息 */
		if (MemCacheInit(memCache, "mem cache", cacheSize->cacheSize, 0)) {
			printk(PART_ERROR "create mem cache failed!\n");
			return -1;
		}
		
		// 设定cachePtr
		cacheSize->memCache = memCache;

		// 指向下一个mem cache
		memCache++;

		// 指向下一个cache size
		cacheSize++;
	}

	return 0;
}

/*
 * __groupAllocObjcet - 在group中分配一个对象
 * @cache: 对象所在的cache
 * @group: 对象所在的group
 * 
 * 在当前cache中分配一个对象
 */
PRIVATE INLINE void *__MemGroupAllocObjcet(struct MemCache *cache, struct MemGroup *group)
{
	void *object;

	// 做一些标志的设定

	
	// 从位图中获取一个空闲的对象
	int idx = BitmapScan(&group->map, 1);
	
	// 分配失败
	if (idx == -1) {
		/* 没有可用对象了 */
		printk(PART_TIP "bitmap scan failed!\n");
		
		return NULL;
	}
		
	// 设定为已经使用
	BitmapSet(&group->map, idx, 1);

	// 获取object的位置
	object = group->objects + idx * cache->objectSize;
	
	// 改变group的使用情况
	group->usingCount++;
	group->freeCount--;
	
	// 判断group是否已经使用完了
	if (group->freeCount == 0) {
		// 从partial的这链表删除
		ListDel(&group->list);
		// 添加到full中去
		ListAddTail(&group->list, &cache->fullGroups);
	}

	return object;
}

/*
 * groupAllocObjcet - 在group中分配一个对象
 * @cache: 对象所在的cache
 * @flags: 分配需要的标志
 * 
 * 在当前cache中分配一个对象
 */
PRIVATE void *GroupAllocObjcet(struct MemCache *cache)
{
	void *object;
	struct MemGroup *group;

	// 存在空闲的就分配并且返回
	struct List *partialList, *node;

	// 检测分配环境

	enum InterruptStatus oldStatus;

ToRetryAllocObject:
	// 要关闭中断，并保存寄存器环境
	oldStatus = InterruptDisable();

	partialList = &cache->partialGroups;

	// 指向partial中的第一个group
	node = partialList->next;

	//printk(PART_TIP "cache size %x\n", cache->objectSize);

	// 如果partial是空的
	if (ListEmpty(partialList)) {
		//printk(PART_TIP "partialList empty\n");

		struct List *freeList;
		freeList = &cache->freeGroups;
		// 如果free是空的
		if (ListEmpty(freeList)) {
			//printk(PART_TIP "free empty\n");

			// 需要创建一个新的group
			goto ToCreateNewgroup;
		}
		// 指向第一个组
		node = freeList->next;

		// 把node从free list中删除
		ListDel(node);

		// 把node添加到partial中去
		ListAddTail(node, partialList);

	}

	//printk("kmalloc: found a node.\n");
	/* 现在node是partial中的一个节点 */
	group = ListOwner(node, struct MemGroup, list);
	//printk(PART_TIP "group %x cache size %x\n", group, cache->objectSize);
	object = __MemGroupAllocObjcet(cache, group);

	// 要恢复中断状态
	InterruptSetStatus(oldStatus);

	return object;

ToCreateNewgroup:
	// 没有group，添加一个新的group

	// 恢复中断状况
	// 要恢复中断状态
	InterruptSetStatus(oldStatus);

	//printk("kmalloc: need a new group.\n");
	// 添加新的group
	if (CreateMemGroup(cache, 0))
		return NULL;	// 如果创建一个group失败就返回

	goto ToRetryAllocObject;
	return NULL;
}

/*
 * kmalloc - 分配一个对象
 * @size: 对象的大小
 * @flags: 分配需要的flags
 * 
 * 分配一个size大小的内存，用flags
 */
PUBLIC void *kmalloc(size_t size, unsigned int flags)
{
	// 如果越界了就返回空
	if (size > MAX_MEM_CACHE_SIZE) {
		printk(PART_WARRING "kmalloc size %d too big!", size);
		return NULL;
	}
		
	// 判断是否有标志
	/*if (!flags) 
		return NULL;*/

	struct CacheSize *cacheSize = cacheSizes;

	while (cacheSize->cacheSize) {
		// 如果找到一个大于等于当前大小的cache，就跳出循环
		if (cacheSize->cacheSize >= size)
			break;
		
		// 指向下一个大小描述
		cacheSize++;
	}
	
	//printk(PART_TIP "des %x cache %x size %x\n", sizeDes, sizeDes->cachePtr, sizeDes->cachePtr->objectSize);
	return GroupAllocObjcet(cacheSize->memCache);
}


/*
 * __GroupFreeObject - 释放一个group对象
 * @cache: 对象所在的cache
 * @object: 对象的指针
 * 
 * 释放group对象，而不是group，group用destory
 */
PRIVATE void __GroupFreeObject(struct MemCache *cache, void *object)
{
	struct MemGroup *group;

	// 获取group

	// 获取页
	struct MemNode *node = Page2MemNode(Vir2Phy(object));

	CHECK_MEM_NODE(node);

	//printk(PART_TIP "OBJECT %x page %x cache %x", object, page, page->groupCache);
	// 遍历cache的3个链表。然后查看group是否有这个对象的地址
	group = MEM_NODE_GET_GROUP(node);

	// 如果查询失败，就返回，代表没有进行释放
	if (group == NULL) 
		Panic("group get from page bad!\n");

	//printk(PART_TIP "get object group %x\n", group);

	// 把group中对应的object释放，也就是把位图清除

	// 找到位图的索引
	int index = (((unsigned char *)object) - group->objects)/cache->objectSize; 
	
	// 检测index是否正确
	if (index < 0 || index > group->map.btmpBytesLen*8)
		Panic("map index bad range!\n");
	
	// 把位图设置为0，就说明它没有使用了
	BitmapSet(&group->map, index, 0);

	int unsing = group->usingCount;
	/*
	DumpMemGroup(group);
	DumpMemNode(node);*/

	//printk("kfree: found group and node.\n");

	// 使用中的对象数减少
	group->usingCount--;
	// 空闲的对象数增加
	group->freeCount++;
	
	// 没有使用中的对象
	if (!group->usingCount) {
		// 把它放到free空闲列表
		ListDel(&group->list);
		ListAddTail(&group->list, &cache->freeGroups);

		//printk("kfree: free to free group.\n");
		//memset(group->map.bits, 0, group->map.btmpBytesLen);
		//printk(PART_TIP "free to cache %x name %s group %x\n", cache, cache->name, group);
	} else if (unsing == cache->objectNumber) {
		// 释放之前这个是满的group，现在释放后，就到partial中去
		ListDel(&group->list);
		ListAddTail(&group->list, &cache->partialGroups);
		
		//printk("kfree: free to partial group.\n");
	}
}

/*
 * GroupFreeObject - 释放一个group对象
 * @cache: 对象所在的cache
 * @object: 对象的指针
 * 
 * 释放group对象，而不是group，group用destory
 */
PRIVATE void GroupFreeObject(struct MemCache *cache, void *object)
{
	// 检测环境

	// 关闭中断
	enum InterruptStatus oldStatus = InterruptDisable();

	__GroupFreeObject(cache, object);

	// 打开中断
	InterruptSetStatus(oldStatus);
}


/*
 * kfree - 释放一个对象占用的内存
 * @object: 对象的指针
 */
PUBLIC void kfree(void *objcet)
{
	if (!objcet)
		return;
	struct MemCache *cache;
	
	// 获取对象所在的页
	struct MemNode *node = Page2MemNode(Vir2Phy(objcet));

	CHECK_MEM_NODE(node);
	
	// 转换成group cache
	cache = MEM_NODE_GET_CACHE(node);

	//DumpMemCache(cache);
	//printk(PART_TIP "get object group cache %x\n", cache);

	// 调用核心函数
	GroupFreeObject(cache, (void *)objcet);
	
}


/*
 * groupDestory - 销毁group
 * @cache: group所在的cache
 * @group: 要销毁的group
 * 
 * 销毁一个group,成功返回0，失败返回-1
 */
PRIVATE int GroupDestory(struct MemCache *cache, struct MemGroup *group)
{
	// 删除链表关系
	ListDel(&group->list);
	
	/* 根据缓冲中记录的对象大小进行不同的设定 */
	if (cache->objectSize < 1024) {
		/* 只释放group所在的内存，因为所有数据都在里面 */
		if (MemCacheFreePages(group))
			return -1;
	} else {
		/* 要释放group和对象所在的页 */
		if (MemCacheFreePages(group->objects))
			return -1;
		if (MemCacheFreePages(group))
			return -1;
	}
	return 0;
}

/**
 * __groupCacheShrink - 收缩内存大小 
 * @cache: 收缩哪个缓冲区的大小
 */
PRIVATE int __MemCacheShrink(struct MemCache *cache)
{
	struct MemGroup *group, *next;

	int ret = 0;

	ListForEachOwnerSafe(group, next, &cache->freeGroups, list) {
		// 销毁成功才计算销毁数量
		//printk("find a free group %x\n", group);
		if(!GroupDestory(cache, group))
			ret++;
	}
	return ret;
}

/**
 * SlabCacheShrink - 收缩内存大小 
 * @cache: 收缩哪个缓冲区的大小
 */
PRIVATE int MemCacheShrink(struct MemCache *cache)
{
	int ret;
	// cache出错的话就返回
	if (!cache) 
		return 0; 

	// 用自旋锁来保护结构
	enum InterruptStatus oldStatus = InterruptDisable();
	// 收缩内存
	ret = __MemCacheShrink(cache);

	// 打开锁
	InterruptSetStatus(oldStatus);

	// 返回收缩了的内存大小
	return ret * cache->objectNumber * cache->objectSize;
}

/**
 * SlabCacheAllShrink - 对所有的cache都进行收缩
 */
PUBLIC int kmshrink()
{
	// 释放了的大小
	size_t size = 0;

	// 指向cache的指针
	struct CacheSize *cacheSize = &cacheSizes[0];

	// 对每一个cache都进行收缩
	while (cacheSize->cacheSize) {
		// 收缩大小
		size += MemCacheShrink(cacheSize->memCache);
		// 指向下一个cache大小描述
		cacheSize++;
	}

	return size;
}

PUBLIC int InitMemCaches()
{
	PART_START("Mem Cache");
	MakeMemCaches();

	/*
	char *a = kmalloc2(32, 0);
	char *b = kmalloc2(512, 0);
	char *c = kmalloc2(1024, 0);
	char *d = kmalloc2(128*1024, 0);
	
	//memset(d, 0, 512*1024);

	a = kmalloc2(32, 0);
	b = kmalloc2(512, 0);
	c = kmalloc2(1024, 0);
	d = kmalloc2(128*1024, 0);

	printk("a=%x, b=%x,c=%x,d=%x,\n", a, b,c,d);

	kfree2(a);
	kfree2(b);
	kfree2(c);
	kfree2(d);
	
	a = kmalloc2(64, 0);
	b = kmalloc2(256, 0);
	c = kmalloc2(4096, 0);
	d = kmalloc2(64*1024, 0);

	printk("a=%x, b=%x,c=%x,d=%x,\n", a, b,c,d);
*/
/*
	int i = 0;

	char *table[10];
	for (i = 0; i < 10; i++) {
		table[i] = kmalloc2(128*1024, 0);
		printk("x=%x\n", table[i]);
	}

	for (i = 0; i < 10; i++) {
		kfree2(table[i]);
	}

	size_t size = kmshrink();
	printk("shrink size %x bytes %d MB\n", size, size/MB);

	for (i = 0; i < 10; i++) {
		table[i] = kmalloc2(128*1024, 0);
		printk("x=%x\n", table[i]);
	}*/

	/*
	size = ShrinkMemCache();
	printk("shrink size %x bytes %d MB\n", size, size/MB);
	*/
/*
	for (i = 0; i < 10; i++) {
		kfree2(table[i]);
	}*/

	PART_END();
	return 0;
}
