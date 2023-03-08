#ifndef _RT_FDT_H
#define _RT_FDT_H

#include <rtthread.h>

/* will be optimized to only u64 or u32 by gcc */
#define IN_64BITS_MODE  (sizeof(void *) == 8)

#define FDT_ROOT_ADDR_CELLS_DEFAULT 1
#define FDT_ROOT_SIZE_CELLS_DEFAULT 1

#define FDT_DTB_ALL_NODES_PATH_SIZE (32 * 1024)
#define FDT_DTB_PAD_SIZE            1024

#define FDT_RET_NO_MEMORY   2
#define FDT_RET_NO_LOADED   1
#define FDT_RET_GET_OK      0
#define FDT_RET_GET_EMPTY   (-1)

#define SEEK_SET	0	/* seek relative to beginning of file */
#define SEEK_CUR	1	/* seek relative to current file position */
#define SEEK_END	2	/* seek relative to end of file */
#define SEEK_DATA	3	/* seek to the next data */
#define SEEK_HOLE	4	/* seek to the next hole */
#define SEEK_MAX	SEEK_HOLE

typedef rt_uint32_t phandle;

struct dtb_memreserve
{
    rt_uint64_t address;
    rt_uint64_t size;
};

struct dtb_header
{
    char root, zero;        /* "/" */
    struct dtb_memreserve *memreserve;
    rt_size_t memreserve_sz;
};

struct dtb_property
{
    const char *name;
    int size;
    void *value;

    struct dtb_property *next;
};

struct dtb_node
{
    union
    {
        const char *name;
        const struct dtb_header *header;
    };
    const char *path;
    phandle handle;

    struct dtb_property *properties;
    struct dtb_node *parent;
    struct dtb_node *child;
    struct dtb_node *sibling;
};

void *fdt_load_from_fs(char *dtb_filename);
void *fdt_load_from_memory(void *dtb_ptr, rt_bool_t is_clone);

rt_size_t fdt_set_linux_cmdline(void *fdt, char *cmdline);
rt_size_t fdt_set_linux_initrd(void *fdt, rt_uint64_t initrd_addr, rt_size_t initrd_size);

rt_size_t fdt_set_dtb_property(void *fdt, char *pathname, char *property_name, rt_uint32_t *cells, rt_size_t cells_size);
rt_size_t fdt_add_dtb_memreserve(void *fdt, rt_uint64_t address, rt_uint64_t size);
rt_size_t fdt_del_dtb_memreserve(void *fdt, rt_uint64_t address);

rt_err_t fdt_get_exec_status();
struct dtb_node *fdt_get_dtb_list(void *fdt);
void fdt_free_dtb_list(struct dtb_node *dtb_node_head);
void fdt_get_dts_dump(struct dtb_node *dtb_node_head);
void fdt_get_enum_dtb_node(struct dtb_node *dtb_node_head, void (callback(struct dtb_node *dtb_node)));

struct dtb_node *fdt_get_dtb_node_by_name_DFS(struct dtb_node *dtb_node, const char *nodename);
struct dtb_node *fdt_get_dtb_node_by_name_BFS(struct dtb_node *dtb_node, const char *nodename);
struct dtb_node *fdt_get_dtb_node_by_path(struct dtb_node *dtb_node, const char *pathname);
struct dtb_node *fdt_get_dtb_node_by_phandle_DFS(struct dtb_node *dtb_node, phandle handle);
struct dtb_node *fdt_get_dtb_node_by_phandle_BFS(struct dtb_node *dtb_node, phandle handle);
void fdt_get_dtb_node_cells(struct dtb_node *dtb_node, int *addr_cells, int *size_cells);
void *fdt_get_dtb_node_property(struct dtb_node *dtb_node, const char *property_name, rt_size_t *property_size);
struct dtb_memreserve *fdt_get_dtb_memreserve(struct dtb_node *dtb_node, int *memreserve_size);
rt_uint8_t fdt_get_dtb_byte_value(void *value);

char *fdt_get_dtb_string_list_value(void *value, int size, int index);
char *fdt_get_dtb_string_list_value_next(void *value, void *end);
rt_uint32_t fdt_get_dtb_cell_value(void *value);

rt_bool_t fdt_get_dtb_node_status(struct dtb_node *dtb_node);
rt_bool_t fdt_get_dtb_node_compatible_match(struct dtb_node *dtb_node, char **compatibles);

#define fdt_string_list(string, ...) ((char *[]){string, ##__VA_ARGS__, RT_NULL})

#define for_each_property_string(node_ptr, property_name, str, size)        \
    for (str = fdt_get_dtb_node_property(node_ptr, property_name, &size),   \
        size += (typeof(size))(rt_ubase_t)str;                              \
        str && *str;                                                        \
        str = fdt_get_dtb_string_list_value_next((void *)str, (void *)(rt_ubase_t)size))

#define for_each_property_cell(node_ptr, property_name, value, list, size)  \
    for (list = fdt_get_dtb_node_property(node_ptr, property_name, &size),  \
        value = fdt_get_dtb_cell_value(list),                               \
        size /= sizeof(rt_uint32_t);                                        \
        size > 0;                                                           \
        value = fdt_get_dtb_cell_value(++list), --size)

#define for_each_property_byte(node_ptr, property_name, value, list, size)  \
    for (list = fdt_get_dtb_node_property(node_ptr, property_name, &size),  \
        value = fdt_get_dtb_byte_value(list);                               \
        size > 0;                                                           \
        value = fdt_get_dtb_byte_value(++list), --size)

#define for_each_node_child(node_ptr)                           \
    for (node_ptr = (node_ptr ? node_ptr->child : RT_NULL);     \
        node_ptr != RT_NULL;                                    \
        node_ptr = node_ptr->sibling)

#define for_each_node_sibling(node_ptr)                         \
    for (node_ptr = (node_ptr ? node_ptr->sibling : RT_NULL);   \
        node_ptr != RT_NULL;                                    \
        node_ptr = node_ptr->sibling)

#endif /* _RT_FDT_H */
