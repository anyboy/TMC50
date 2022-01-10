#ifndef __PAGE_INNER_H__
#define __PAGE_INNER_H__

#if 1
#define RAM_MPOOL0_MAX_NUM      (28)
#define RAM_MPOOL1_MAX_NUM      (0)
#else
#define RAM_MPOOL0_MAX_NUM      (65 + 9)
#define RAM_MPOOL1_MAX_NUM      (50)
#endif

extern int __ram_mpool0_start, __ram_mpool0_size, __ram_mpool0_num;
//extern int __ram_core_idle_page_start, __ram_core_idle_page_num;
//extern int __ram_mpool0_num_add_idle_num;
extern int __ram_dsp_start, __ram_dsp_size, __ram_mpool1_num;
extern int __ram_adata_mpool0_index;
extern int __ram_dsp_mpool1_num;
extern int __ram_hw_start;
extern int __ram_hw_size;

#define PAGE_SIZE   (1UL << CONFIG_POOL_PAGE_SHIFT)
#define PAGE_MASK   (~(PAGE_SIZE-1))

#define FREE_INDEX_FREE_FLAG    (0xFF)
#define FREE_INDEX_USED_FLAG    (0x7F)

#define POOL0_ADDR ((void *)&__ram_mpool0_start)
#define POOL0_SIZE ((int)&__ram_mpool0_size)
//#define POOL0_NUM ((int)&__ram_mpool0_num_add_idle_num)
#define POOL0_NUM ((int)&__ram_mpool0_num)
#define DSP_ADDR ((void *)&__ram_dsp_start)
#define DSP_SIZE ((int)&__ram_dsp_size)
#define POOL1_DSP_NUM  ((int)&__ram_dsp_mpool1_num)
#define HW_RAM_ADDR  ((void *)&__ram_hw_start)
#define HW_RAM_SIZE  ((int)&__ram_hw_size)
#define POOL1_NUM ((int)&__ram_mpool1_num)

#define SIZE2PAGE(size) ((size + PAGE_SIZE - 1) >> CONFIG_POOL_PAGE_SHIFT)
#define PAGE2SIZE(page) (page << CONFIG_POOL_PAGE_SHIFT)


enum pagepool_zone
{
    zone_ram_cache,
    zone_ram_dsp,
};

struct list_index {
    unsigned char next_index, prev_index;   //bit7 is zone, bit6~0 is index
};

struct page
{
    struct list_index lru;
    unsigned short ibank_no;
};


extern struct list_index freelist;
extern struct page pagepool0[RAM_MPOOL0_MAX_NUM];
//extern struct page pagepool1[RAM_MPOOL1_MAX_NUM];
extern struct page *pagepool[1];
extern const int pagepool_size[1];
extern unsigned char freepage_num[1];

#define INDEX_ZONE(index) (((index) & 0x80) != 0)
#define INDEX2PAGE(index) (index&0x7f)
#define NODE(index)  ((index) == (unsigned char)FREE_INDEX_FREE_FLAG ? &freelist : (struct list_index *)&pagepool[INDEX_ZONE((index))][INDEX2PAGE(index)])
#define PAGE(index) ((struct page *)NODE((index)))
#define PAGE_IN_FREELIST(page) ((page)->lru.prev_index != (unsigned char)FREE_INDEX_USED_FLAG)
#define freelist_empty() (freelist.prev_index == (unsigned char)FREE_INDEX_FREE_FLAG && freelist.next_index == (unsigned char)FREE_INDEX_FREE_FLAG)


typedef struct
{
    struct list_index *freelist;
    int continus_start_alloc_page_num;
    int (*pagepool_is_page_in_freelist)(struct page *page);
    int (*pagepool_cache_ibank_into_freepage_cb)(unsigned short ibank_no, void *ibank_addr);
    void (*pagepool_freelist_del_cb)(unsigned char index);
    void *(*pagepool_convert_index_to_addr)(unsigned char index);
}page_alloc_ctx_t;

typedef struct
{
    struct list_index *freelist;
    //int *pagepool_size;
    //unsigned char *freepage_num;
    unsigned char (*pagepool_convert_addr_to_pageindex)(void *addr);
    struct page* (*pagepool_get_page_by_index)(unsigned char index);
    int (*pagepool_is_page_in_freelist)(struct page *page);
    void (*pagepool_freelist_add)(unsigned char new_index);
}page_free_ctx_t;

extern void * rom_pagepool_alloc_pages(unsigned int page_num, unsigned int zone_index, \
            const int *pagepool_size,
            unsigned char *freepage_num,
            struct page **pagepool,
            page_alloc_ctx_t * ctx);

extern int rom_pagepool_free_pages(void *addr, unsigned int page_num, unsigned int zone_index,\
            const int *pagepool_size,
            unsigned char *freepage_num,
            page_free_ctx_t *ctx);
int pagepool_init(void);
void pagepool_freelist_add(unsigned char new_index);
void pagepool_freelist_del(unsigned char index);
int pagepool_get_zone_by_addr(void *addr);
int pagepool_is_page_in_freelist(struct page *page);
int pagepool_cache_ibank_into_freepage(unsigned short ibank_no, void *ibank_addr);

unsigned char pagepool_convert_addr_to_pageindex(void *addr);
void *pagepool_convert_index_to_addr(unsigned char index);

void * pagepool_alloc_pages(unsigned int page_num);
#define pagepool_alloc_page() pagepool_alloc_pages(1)

int pagepool_free_pages(void *page, unsigned int page_num);
#define pagepool_free_page(page) pagepool_free_pages(page, 1)

void * dspram_alloc_range(void *dsp_ram_start, unsigned int dsp_ram_size);
int dspram_free_range(void *dsp_ram_start, unsigned int dsp_ram_size);

int pfree(void *addr, unsigned int page_num, void *caller);
void * pmalloc(unsigned int size, void *caller);
#endif /*__PAGE_INNER_H__*/

