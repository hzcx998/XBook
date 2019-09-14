/*
 * file:		mm/slab.c
 * auther:		Jason Hu
 * time:		2019/7/11
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/arch.h>
#include <book/slab.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/math.h>

/*
 * 如果是32位系统就有13个，不然就12个（少一个32字节的）
 * 通过页的大小来选择cache数量
 */
#if PAGE_SIZE == 4096
	#define MAX_SLAB_CACHE_NR 13
#else
	#define MAX_SLAB_CACHE_NR 12
#endif

/* 
 * 通过页的大小来选择slab cache的最小的大小
 * 如果是32位的系统，那么就选择16，如果是位的
 * 就选择
 */
#if PAGE_SIZE == 4096
	#define MIN_SLAB_CACHE_SIZE 32
#else
    #define MIN_SLAB_CACHE_SIZE 64
#endif

/* 最开始的slabcache */
struct SlabCache slabCacheTable[MAX_SLAB_CACHE_NR];

/* 专门用来储存slab的cache，在SlabMangement中从里面获取一个slab */
struct SlabCache cacheForSlab;

/*
 * cacheSizes - cache大小的描述
 */
PRIVATE struct CacheSizeDescription cacheSizes[] = {
	#if PAGE_SIZE == 4096
	{32, NULL},
	#endif
	{64, NULL},
	{128, NULL},
	{256, NULL},
	{512, NULL},
	{1024, NULL},	// 1KB
	{2048, NULL},	
	{4096, NULL},	// 4KB
	{8*1024, NULL},
	{16*1024, NULL},
	{32*1024, NULL},
	{64*1024, NULL},	// 64KB
	{128*1024, NULL},	// 128KB
	{0, NULL},		// 用于索引判断结束
};

/* 在这里声明一下 */
PRIVATE void *SlabAllocObjcet(struct SlabCache *cache, unsigned int flags);
PRIVATE void SlabFreeObject(struct SlabCache *cache, void *object);

/**
 * SlabInitWithAlloc - 初始化一个cache中的slab
 * @cache: slab所在的cache
 * @slab: 要初始化的slab
 * @flags: slab的标志
 * @alloc: 分配地址（用于存放位图和对象）
 * 
 * 返回分配后的alloc值，用于下一次分配。
 * slab是静态的，不能销毁
 */
PRIVATE INIT int 
SlabInitWithAlloc(struct SlabCache *cache, struct Slab *slab, flags_t flags, unsigned int alloc)
{
	// 把slab添加到free链表
	ListAdd(&slab->list, &cache->slabsFree);

	// 设定位图
	slab->map.btmpBytesLen = cache->objectNumber/8;
	slab->map.bits = (unsigned char *)alloc;
	alloc += slab->map.btmpBytesLen;

	BitmapInit(&slab->map);

	/* 对分配地址进行页对齐，那么对象就也是也对齐的。
	页对齐只有放在分配对象之前才会有效果 */ 
	alloc = PAGE_ALIGN(alloc);

	// 设定对象
	slab->objects = (unsigned char *)alloc;
	alloc += cache->objectNumber * cache->objectSize;
	slab->usingCount = 0;
	slab->freeCount = cache->objectNumber;
	slab->flags =  flags;

	// 把页标记slab cache
	int i = cache->objectNumber;
	unsigned char *object = slab->objects;

	struct Page *page;
	// 标记全部页
	while (i--) {
		page = PhysicAddressToPage(VirtualToPhysic((unsigned int)object));
		CHECK_PAGE(page);
		PAGE_MARK_SLAB_CACHE(page, cache, slab);
		object += cache->objectSize;
	}
	return alloc;
}

/**
 * SlabCacheInit - 初始化一个slab cache
 * @cache: 要初始化的cache
 * @name: cache的名字
 * @size: cache中对象的大小
 * @flags: cache的标志
 * @gflags: cache中获取页的标志
 * 
 * 初始化一个静态的cache
 */
PRIVATE INIT void 
SlabCacheInit(struct SlabCache *cache, char *name, size_t size, flags_t flags, flags_t gflags)
{
	// 初始化链表
	INIT_LIST_HEAD(&cache->slabsFull);
	INIT_LIST_HEAD(&cache->slabsPartial);
	INIT_LIST_HEAD(&cache->slabsFree);

	// 保存slab对象数量
	cache->objectNumber = SLAB_OBJECTS_SIZE/size;

	// 初始化大小
	cache->objectSize = size;

	// 设定cache的标志
	cache->flags = flags;
	
	// 因为这里固定1M的大小，对应的order是8
	cache->slabSizeOrder = 8;
	cache->slabGetPageFlags = gflags;
	
	// 初始化同级cache的链表
	cache->next = NULL;

	// 构造和析构都是空
	cache->Constructor = NULL;
	cache->Desstructor = NULL;
	
	// 设置名字
	memset(cache->name, 0, SLAB_CACHE_NAME_LEN);
	strcpy(cache->name, name);
}

/* 
 * SlabCacheAllInit - 初始化整个slab cache环境
 */
PRIVATE INIT void SlabCacheAllInit()
{
	PART_START("Slab cache");

	// 指向cache大小的指针
	struct CacheSizeDescription *cacheSizePtr = cacheSizes;

	// 指向slabCacheTable
	struct SlabCache *slabCache = &slabCacheTable[0];
	
	// 显示slab cache的一些信息
	#ifdef CONFI_SLAB_DEBUG
		printk("\n" PART_TIP "slabCacheTable addr %x slab SlabCache size %d\n", slabCache, sizeof(struct SlabCache));
	#endif

	// 先分配2M作为一个连续的内存块
	unsigned int alloc = (unsigned int )PhysicToVirtual(GetFreePages(GFP_STATIC, MAX_ORDER - 1));
	if (alloc == 0)
		Panic(" |- get free page for alloc failed!\n");
	
	// 记录分配开始的地址
	unsigned int start = alloc;

	// ----初始化cacheForSlab----

	// 初始化cache for slabq
	//InitCacheForSlab();
	SlabCacheInit(&cacheForSlab, "cache for slab", 
		sizeof(struct Slab) + (SLAB_OBJECTS_SIZE/MIN_SLAB_CACHE_SIZE)/8, 
		SLAB_CACHE_FLAGS_STATIC, GFP_STATIC);
	
	// 为它分配slab
	struct Slab *slab;
	// slab指向alloc地址，然后alloc增加slab的大小，就实现了分配一段内存
	slab = (struct Slab *) alloc;
	alloc += sizeof(struct Slab);

	// 初始化slab信息，这样，这个slab才能使用
	alloc = SlabInitWithAlloc(&cacheForSlab, slab, SLAB_FLAGS_STATIC, alloc);

	/*
	printk("cache %s object size %d number %d slabSizeOrder %d \n", \
		cacheForSlab.name, cacheForSlab.objectSize, cacheForSlab.objectNumber, cacheForSlab.slabSizeOrder);

	printk("slab %x bits %x len %d objects %x usingCount %d freeCount %d\n", \
		slab, slab->map.bits, slab->map.btmpBytesLen, slab->objects, slab->usingCount, slab->freeCount);
	 */
	
	// ----初始化基础的cache----

	// 如果没有遇到大小为0的cache，就会把cache初始化
	while (cacheSizePtr->cacheSize) {

		// 初始化一个cache
		SlabCacheInit(slabCache, "slab cache", cacheSizePtr->cacheSize, SLAB_CACHE_FLAGS_STATIC, GFP_STATIC);
		
		// 分配slab
		slab = (struct Slab *) alloc;
		alloc += sizeof(struct Slab);

		// 初始化一个slab
		alloc = SlabInitWithAlloc(slabCache, slab, SLAB_FLAGS_STATIC, alloc);
		/* 
		printk(PART_TIP "cache %s object size %d number %d slabSizeOrder %d \n", \
			slabCache->name, slabCache->objectSize, slabCache->objectNumber, slabCache->slabSizeOrder);

		printk(PART_TIP "slab %x bits %x len %d objects %x usingCount %d freeCount %d\n", \
			slab, slab->map.bits, slab->map.btmpBytesLen, slab->objects, slab->usingCount, slab->freeCount);
		*/
	
		// 设定cachePtr
		cacheSizePtr->cachePtr = slabCache;

		// 指向下一个cache
		slabCache++;

		// 指向下一个cache
		cacheSizePtr++;
	}
	// Spin("SlabInitWithAlloc");
	#ifdef CONFI_SLAB_DEBUG
		printk(PART_TIP "memory alloc end %x size %x\n", alloc, alloc - start);
	#endif
	// 把占用的总大小减去第一次分配的大小后，再摘取对应大小的页，以字节为单位的数量
	unsigned int used =  (alloc - start) - 2*MB;

	// 转换成2MB为单位的数量
	used = DIV_ROUND_UP(used, 2*MB);

	// 摘取对应的页下来
	while (used--) 
		GetFreePages(GFP_STATIC, MAX_ORDER - 1);

	
	// 打印slab cache
	#ifdef CONFI_SLAB_DEBUG
		int i;
		for (i = 0; i < MAX_SLAB_CACHE_NR; i++) {
			printk(PART_TIP "%d addr %x size %d num %d\n", i, &slabCacheTable[i], 
					slabCacheTable[i].objectSize, slabCacheTable[i].objectNumber);
		}
	#endif
	
	PART_END();
}

/* 
 * SlabGetPages - 从slab中获取页
 * @cache: 要用哪个cache的信息
 * @flags: 新的标志
 * 
 * 这是slab和buddy的接口，分配物理页给slab的对象
 */
PRIVATE INLINE void *SlabGetPages(struct SlabCache *cache, unsigned int flags)
{
	void *addr;

	// 添加新的标志
	flags |= cache->slabGetPageFlags;
	// 调用buddy的获取页
	addr = (void *)PhysicToVirtual(GetFreePages(flags, cache->slabSizeOrder));
	// 返回
	return addr;
}

/* 
 * SlabFreePages - 从slab中释放页
 * @cache: 要用哪个cache的信息
 * @flags: 要释放的地址
 * 
 * 这是slab和buddy的接口，释放slab中对象分配的页
 */
PRIVATE INLINE void SlabFreePages(struct SlabCache *cache, void *addr)
{
	unsigned long i = (1 << cache->slabSizeOrder);

	address_t paddr = VirtualToPhysic((unsigned int)addr);

	struct Page *page = PhysicAddressToPage(paddr);
	//printk(PART_TIP "pages %d paddr %x page %x", i, paddr, page);
	while (i--) {
		// 清除页上面的slab标志
		PAGE_CLEAR_SLAB_CACHE(page);
		page++;
	}
	
	// 调用buddy的释放页，传入物理地址
	FreePages((unsigned int)paddr, cache->slabSizeOrder);
}


/*
 * SlabAllocFromCache - 从cache for slab中分配一个slab
 * @cache: slab所在的cache
 * @flags: 分配一个对象所需要的flags
 * 
 * 从CacheForSlab中分配一个slab空间，释放的时候也需要释放到CacheForSlab中
 */
PRIVATE INLINE struct Slab *SlabAllocFromCache(struct SlabCache *cache, flags_t flags)
{
	struct Slab *slab;

	// 直接分配一个slab
	slab = SlabAllocObjcet(&cacheForSlab, flags);	// 是的话就从cacheForSlab中分配一个
	return slab;
}

/*
 * SlabInitObjects - 初始化slab中的对象
 * @cache: 对象所在的cache
 * @slab: 对象所在的slab
 * @flags: 初始化时需要传入的falgs给构建函数
 * 
 * 通过构建函数来初始化所有的slab中的对象
 */
PRIVATE INLINE void SlabInitObjects(struct SlabCache *cache, struct Slab *slab, flags_t flags)
{
	int i;
	void *object;
	for (i = 0; i < cache->objectNumber; i++) {
		object = slab->objects + i * cache->objectSize;
		if (cache->Constructor)
			cache->Constructor(object, cache, flags);  
	}
}

/*
 * SlabCreate - 创建一个新的slab
 * @cache: slab所在的cache
 * @flags:创建的标志
 * 
 * 如果成功返回，失败返回-1
 */
PRIVATE int SlabCreate(struct SlabCache *cache, flags_t flags)
{
	struct Slab *slab;
	struct Page *page;
	void *objects;

	
	// 为对象组分配空间
	objects = SlabGetPages(cache, flags);
	if (objects == NULL)
		goto ToFailed;

	// 为slab分配内存
	slab = SlabAllocFromCache(cache, flags);
	if (slab == NULL)
		goto ToFreePages;

	//printk(PART_TIP "alloc cache %x slab %x\n", cache, slab);

	// ----初始化slab内容----

	// 初始化对象
	slab->objects = objects;

	// 初始化位图
	slab->map.btmpBytesLen = cache->objectNumber/8;
	slab->map.bits = (unsigned char *)(slab + 1);	// 位图默认放到slab的后面
	BitmapInit(&slab->map);
	
	// 初始化使用情况
	slab->freeCount = cache->objectNumber;
	slab->usingCount = 0;

	// 给动态创建的添加dynamic标志
	slab->flags = SLAB_FLAGS_DYNAMIC;

		// 添加到链表
	ListAddTail(&slab->list, &cache->slabsFree);

	// ----把cache和slab都标记到每一个对象所在的页
	int i = 1 << cache->slabSizeOrder;
	page = PhysicAddressToPage(VirtualToPhysic((unsigned int)objects));
	CHECK_PAGE(page);
	do {
		PAGE_MARK_SLAB_CACHE(page, cache, slab);
		page++;
	} while (--i);

	// 初始化对象
	SlabInitObjects(cache, slab, flags);
	/*
	printk(PART_TIP "SlabCreate slab %x bits %x len %d objects %xn", \
		slab, slab->map.bits, slab->map.btmpBytesLen, slab->objects);
	*/
	// 创建成功
	return 0;

ToFreePages:
	// 释放对象组的页
	SlabFreePages(cache, objects);

ToFailed:

	return -1;
}

/*
 * __SlabAllocObjcet - 在slab中分配一个对象
 * @cache: 对象所在的cache
 * @slab: 对象所在的slab
 * 
 * 在当前cache中分配一个对象
 */
PRIVATE INLINE void *__SlabAllocObjcet(struct SlabCache *cache, struct Slab *slab)
{
	void *object;

	// 做一些标志的设定

	// 改变slab的使用情况
	slab->usingCount++;
	slab->freeCount--;
	
	// 从位图中获取一个空闲的对象
	int idx = BitmapScan(&slab->map, 1);
	
	// 分配失败
	if (idx == -1)
		Panic(PART_TIP "bitmap scan failed!\n");
		
	// 设定为已经使用
	BitmapSet(&slab->map, idx, 1);

	// 获取object的位置
	object = slab->objects + idx * cache->objectSize;

	// 判断slab是否已经使用完了
	if (slab->freeCount == 0) {
		// 从partial的这链表删除
		ListDel(&slab->list);
		// 添加到full中去
		ListAddTail(&slab->list, &cache->slabsFull);
	}

	// 把object所在的页标记上slab cache
	struct Page *page = PhysicAddressToPage(VirtualToPhysic((unsigned int)object));
	if (!page)
		Panic("Object to Page failed!\n");
	
	return object;
}
/*
 * SlabAllocObjcet - 在slab中分配一个对象
 * @cache: 对象所在的cache
 * @flags: 分配需要的标志
 * 
 * 在当前cache中分配一个对象
 */
PRIVATE void *SlabAllocObjcet(struct SlabCache *cache, unsigned int flags)
{
	void *object;
	struct Slab *slab;
	// 存在空闲的就分配并且返回
	struct List *partialList, *node;

	// 检测分配环境


ToRetryAllocObject:
	// 要关闭中断，并保存寄存器环境

	partialList = &cache->slabsPartial;

	// 指向partial中的第一个slab
	node = partialList->next;

	//printk(PART_TIP "cache size %x\n", cache->objectSize);

	// 如果partial是空的
	if (ListEmpty(partialList)) {
		//printk(PART_TIP "partialList empty\n");

		struct List *freeList;
		freeList = &cache->slabsFree;
		// 如果free是空的
		if (ListEmpty(freeList)) {
			//printk(PART_TIP "free empty\n");
	
			// 需要创建一个新的slab
			goto ToCreateNewSlab;
		}
		node = freeList->next;

		// 把node从free list中删除
		ListDel(node);

		// 把node添加到partial中去
		ListAddTail(node, partialList);

	}
	
	slab = ListOwner(node, struct Slab, list);
	//printk(PART_TIP "slab %x cache size %x\n", slab, cache->objectSize);
	object = __SlabAllocObjcet(cache, slab);

	// 要打开中断

	return object;

ToCreateNewSlab:
	// 没有slab，添加一个新的slab

	// 恢复中断状况

	// 添加新的slab
	if (SlabCreate(cache, flags))
		return NULL;	// 如果创建一个slab失败就返回

	goto ToRetryAllocObject;
	return NULL;
}


/*
 * __SlabFreeObject - 释放一个slab对象
 * @cache: 对象所在的cache
 * @object: 对象的指针
 * 
 * 释放slab对象，而不是slab，slab用destory
 */
PRIVATE void __SlabFreeObject(struct SlabCache *cache, void *object)
{
	struct Slab *slab;

	// 获取slab

	// 获取页
	struct Page *page = PhysicAddressToPage(VirtualToPhysic((unsigned int)object));

	CHECK_PAGE(page);

	//printk(PART_TIP "OBJECT %x page %x cache %x", object, page, page->slabCache);
	// 遍历cache的3个链表。然后查看slab是否有这个对象的地址
	slab = PAGE_GET_SLAB(page);

	// 如果查询失败，就返回，代表没有进行释放
	if (slab == NULL) 
		Panic("slab get from page bad!\n");

	//printk(PART_TIP "get object slab %x\n", slab);

	// 把slab中对应的object释放，也就是把位图清除

	// 找到位图的索引
	int index = (((unsigned char *)object) - slab->objects)/cache->objectSize; 
	
	// 检测index是否正确
	if (index < 0 || index > slab->map.btmpBytesLen*8)
		Panic("map index bad range!\n");
	
	// 把位图设置为0，就说明它没有使用了
	BitmapSet(&slab->map, index, 0);

	int unsing = slab->usingCount;

	// 使用中的对象数减少
	slab->usingCount--;
	// 空闲的对象数增加
	slab->freeCount++;
	
	// 没有使用中的对象
	if (!slab->usingCount) {
		// 把它放到free空闲列表
		ListDel(&slab->list);
		ListAddTail(&slab->list, &cache->slabsFree);

		//memset(slab->map.bits, 0, slab->map.btmpBytesLen);
		//printk(PART_TIP "free to cache %x name %s slab %x\n", cache, cache->name, slab);
	} else if (unsing == cache->objectNumber) {
		// 释放之前这个是满的slab，现在释放后，就到partial中去
		ListDel(&slab->list);
		ListAddTail(&slab->list, &cache->slabsPartial);
	}
}

/*
 * SlabFreeObject - 释放一个slab对象
 * @cache: 对象所在的cache
 * @object: 对象的指针
 * 
 * 释放slab对象，而不是slab，slab用destory
 */
PRIVATE INLINE void SlabFreeObject(struct SlabCache *cache, void *object)
{
	// 检测环境

	// 关闭中断

	__SlabFreeObject(cache, object);

	// 打开中断
}


/*
 * SlabDestory - 销毁slab
 * @cache: slab所在的cache
 * @slab: 要销毁的slab
 * 
 * 销毁一个slab,成功返回0，失败返回-1
 */
PRIVATE int SlabDestory(struct SlabCache *cache, struct Slab *slab)
{

	// 如果有析构函数，那么在销毁一个
	if (cache->Desstructor) {
		// 获取slab中的对象
		int i;
		for (i = 0; i < cache->objectNumber; i++) {		
			void *object = slab->objects + i * cache->objectSize;
			// 检测object是否正确

			// 调用析构函数
			cache->Desstructor(object, cache, 0);
		}
	}
	//printk("slab is %x objects %x", slab, slab->objects);
	// 如果SLAB是静态定义的，那么就不能销毁
	if (slab->flags&SLAB_CACHE_FLAGS_STATIC)
		return -1;
	
	// 删除链表关系
	ListDel(&slab->list);
	//printk("- list del -");
	// 释放对象所占用的内存
	SlabFreePages(cache, slab->objects);
	//printk("- SlabFreePages -");
	// ----释放slab结构所占用的内存----

	// 判断slab是否和对象群在一个区域里面
	SlabFreeObject(&cacheForSlab, slab);
	//printk("- SlabFree -");
	return 0;
}

/**
 * __SlabCacheShrink - 收缩内存大小 
 * @cache: 收缩哪个缓冲区的大小
 */
PRIVATE INLINE int __SlabCacheShrink(struct SlabCache *cache)
{
	struct Slab *slab, *next;

	int ret = 0;
	//printk("start shrink\n");
	// 查找每一个
	ListForEachOwnerSafe(slab, next, &cache->slabsFree, list) {
		// 销毁成功才计算销毁数量
		//printk("find a free slab %x\n", slab);
		if(!SlabDestory(cache, slab))
			ret++;
	}
	return ret;
}

/**
 * SlabCacheShrink - 收缩内存大小 
 * @cache: 收缩哪个缓冲区的大小
 */
PRIVATE int SlabCacheShrink(struct SlabCache *cache)
{
	int ret;
	// cache出错的话就返回
	if (!cache) 
		return 0; 

	// 用自旋锁来保护结构

	// 收缩内存
	ret = __SlabCacheShrink(cache);

	// 打开锁

	// 返回收缩了的内存大小
	return ret << cache->slabSizeOrder;
}

/**
 * SlabCacheAllShrink - 对所有的cache都进行收缩
 */
PUBLIC int SlabCacheAllShrink()
{
	// 释放了的大小
	size_t size = 0;

	// 指向cache的指针
	struct CacheSizeDescription *cacheSizePtr = &cacheSizes[0];

	// 对每一个cache都进行收缩
	while (cacheSizePtr->cacheSize) {
		// 收缩大小
		size += SlabCacheShrink(cacheSizePtr->cachePtr);
		// 指向下一个cache大小描述
		cacheSizePtr++;
	}

	return size;
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
	if (size > MAX_SLAB_CACHE_SIZE)
		return NULL;
	
	// 判断是否有标志
	if (!flags) 
		return NULL;

	// 获取大小描述
	struct CacheSizeDescription *sizeDes = cacheSizes;

	// 如果size描述没有到最后，就一直循环查找
	while (sizeDes->cacheSize) {
		// 如果找到一个大于等于当前大小的cache，就跳出循环
		if (sizeDes->cacheSize >= size)
			break;
			
		// 指向下一个大小描述
		sizeDes++;
	}
	//printk(PART_TIP "des %x cache %x size %x\n", sizeDes, sizeDes->cachePtr, sizeDes->cachePtr->objectSize);
	return SlabAllocObjcet(sizeDes->cachePtr, flags);
}

/*
 * kfree - 释放一个对象占用的内存
 * @object: 对象的指针
 */
PUBLIC void kfree(void *objcet)
{
	struct SlabCache *cache;
	if (!objcet)
		return;
	
	// 关闭中断
	
	// 获取对象所在的页
	struct Page *page = PhysicAddressToPage(VirtualToPhysic((unsigned int)objcet));
	CHECK_PAGE(page);

	// 转换成slab cache
	cache = PAGE_GET_SLAB_CACHE(page);

	//printk(PART_TIP "get object slab cache %x\n", cache);

	// 调用核心函数
	SlabFreeObject(cache, (void *)objcet);
	// 打开中断

}
/* 
 * SlabCacheTest 
 */
PRIVATE void SlabCacheTest()
{
	
	/*unsigned int *a = kmalloc(128*KB, 0);
	if (a == NULL)
		Spin(PART_TIP "void ptr");

	memset(a, 0, 128*KB);
	printk(PART_TIP "%x", a);

	kfree(a);

	a = kmalloc(128*KB, 0);
	if (a == NULL)
		Spin(PART_TIP "void ptr");

	memset(a, 0, 128*KB);
	printk(PART_TIP "%x", a);

	kfree(a);
	 *//*
	SlabCreate(&slabCacheTable[10], GFP_STATIC);
	SlabCreate(&slabCacheTable[10], GFP_STATIC);

	struct List *list = slabCacheTable[10].slabsFree.next;

	ListForEach (list, &slabCacheTable[10].slabsFree) {
		printk(PART_TIP "list %x\n", list);
	}
	
	list = slabCacheTable[10].slabsFree.next;
	list = list->next;
	struct Slab *slab, *next;
	slab = ListOwner(list, struct Slab, list);
	printk(PART_TIP "get slab %x\n", slab);

	ListForEachOwnerSafe (slab, next, &slabCacheTable[10].slabsFree, list) {
		printk("!slab %x", slab);
		SlabDestory(&cacheForSlab, slab);
	}
	
	ListForEach (list, &slabCacheTable[10].slabsFree) {
		printk(PART_TIP "list %x\n", list);
	}*/
	int table[31];
	int i; 
	for (i = 0; i < 30; i++) {
		table[i] = (int )kmalloc(4*KB, GFP_STATIC);
		printk("addr %x\n", table[i]);
	}
	for (i = 0; i < 30; i++) {
		//printk("free %d- ", i);
		kfree((void *)table[i]);
	}
	// 查看队列状况
	/*struct List *list;
	printk("list condition\n");
	ListForEach (list, &slabCacheTable[12].slabsFree) {
		printk(PART_TIP "slabsFree %x\n", list);
	}
	ListForEach (list, &slabCacheTable[12].slabsPartial) {
		printk(PART_TIP "slabsPartial %x\n", list);
	}
	ListForEach (list, &slabCacheTable[12].slabsFull) {
		printk(PART_TIP "slabsFull %x\n", list);
	}*/

	
/*
	for (i = 0; i < 30; i++) {
		table[i] = kmalloc(128*KB, GFP_STATIC);
	}*/
	/*
	int shrinkSize = SlabCacheAllShrink();
	printk(PART_TIP "the size we shrink is %x",shrinkSize);
	 */
	//Spin("SlabTest");
	/*
	i = 17;

	while (--i >= 5) {
		a = kmalloc(pow(2, i), GFP_STATIC);
		if (a == NULL)
			Spin(PART_TIP "void ptr");
		memset(a, 0, pow(2, i));
		printk("%x \n", a);
		kfree(a);
	}
	i = 17;

	while (--i >= 5) {
		a = kmalloc(pow(2, i), GFP_STATIC);
		if (a == NULL)
			Spin(PART_TIP "void ptr");
		memset(a, 0, pow(2, i));
		printk("%x \n", a);
		kfree(a);

	}
	 */
	
}

/* 
 * InitSlabCache - 初始化slab cache的地方
 */
PUBLIC INIT void InitSlabCacheManagement()
{
	PART_START("Slab");

	SlabCacheAllInit();

	// SlabCacheTest();
	

	PART_END();
}