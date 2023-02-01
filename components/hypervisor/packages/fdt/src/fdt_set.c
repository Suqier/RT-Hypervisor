#include "libfdt/libfdt.h"
#include "fdt.h"

static rt_off_t fdt_find_and_add_subnode(void *fdt, char* name)
{
    rt_off_t chosen_offset = 0;

    chosen_offset = fdt_subnode_offset(fdt, 0, name);

    if (chosen_offset == -FDT_ERR_NOTFOUND)
    {
        chosen_offset = fdt_add_subnode(fdt, 0, name);
    }

    return chosen_offset;
}

rt_size_t fdt_set_linux_cmdline(void *fdt, char *cmdline)
{
    rt_off_t chosen_offset;
    rt_size_t cmdline_size;

    if (cmdline == RT_NULL || fdt == RT_NULL)
    {
        goto end;
    }

    chosen_offset = fdt_find_and_add_subnode(fdt, "chosen");
    cmdline_size = rt_strlen(cmdline);

    /* install bootargs */
    if (chosen_offset >= 0 || chosen_offset == -FDT_ERR_EXISTS)
    {
        if (fdt_setprop(fdt, chosen_offset, "bootargs", cmdline, cmdline_size) < 0)
        {
            fdt_open_into(fdt, fdt, fdt_totalsize(fdt) + FDT_DTB_PAD_SIZE);
            fdt_setprop(fdt, chosen_offset, "bootargs", cmdline, cmdline_size);
        }
    }

end:
    return fdt_totalsize(fdt);
}

rt_size_t fdt_set_linux_initrd(void *fdt, rt_uint64_t initrd_addr, rt_size_t initrd_size)
{
    rt_uint64_t addr, size_ptr;
    rt_off_t chosen_offset;
    int i;

    if (fdt == RT_NULL)
    {
        goto end;
    }

    chosen_offset = fdt_find_and_add_subnode(fdt, "chosen");

    /* update the entry */
    for (i = fdt_num_mem_rsv(fdt) - 1; i >= 0; --i)
    {
        fdt_get_mem_rsv(fdt, i, &addr, &size_ptr);
        if (addr == initrd_addr)
        {
            fdt_del_mem_rsv(fdt, i);
            break;
        }
    }

    /* add the memory */
    if (fdt_add_mem_rsv(fdt, initrd_addr, initrd_size) < 0)
    {
        /* move the memory */
        fdt_open_into(fdt, fdt, fdt_totalsize(fdt) + FDT_DTB_PAD_SIZE);
        if (fdt_add_mem_rsv(fdt, initrd_addr, initrd_size) < 0)
        {
            goto end;
        }
    }

    /* install initrd */
    if (chosen_offset >= 0 || chosen_offset == -FDT_ERR_EXISTS)
    {
        chosen_offset = fdt_path_offset(fdt, "/chosen");

        if (IN_64BITS_MODE)
        {
            fdt_setprop_u64(fdt, chosen_offset, "linux,initrd-start", initrd_addr);
            fdt_setprop_u64(fdt, chosen_offset, "linux,initrd-end", initrd_addr + initrd_size);
        }
        else
        {
            fdt_setprop_u32(fdt, chosen_offset, "linux,initrd-start", initrd_addr);
            fdt_setprop_u32(fdt, chosen_offset, "linux,initrd-end", initrd_addr + initrd_size);
        }
    }

end:
    return fdt_totalsize(fdt);
}

rt_size_t fdt_set_dtb_property(void *fdt, char *pathname, char *property_name, rt_uint32_t *cells, rt_size_t cells_size)
{
    int node_off;

    if (fdt == RT_NULL)
    {
        goto end;
    }

    node_off = fdt_path_offset(fdt, pathname);

    if (node_off >= 0 && cells_size != 0)
    {
        fdt_setprop(fdt, node_off, property_name, cells, cells_size);
    }

end:
    return fdt_totalsize(fdt);
}

rt_size_t fdt_add_dtb_memreserve(void *fdt, rt_uint64_t address, rt_uint64_t size)
{
    if (fdt == RT_NULL)
    {
        goto end;
    }

    fdt_add_mem_rsv(fdt, address, size);

end:
    return fdt_totalsize(fdt);
}

rt_size_t fdt_del_dtb_memreserve(void *fdt, rt_uint64_t address)
{
    int i;
    int num_mem_rsvmap;
    rt_uint32_t off_mem_rsvmap;
    struct fdt_reserve_entry *rsvmap;

    if (fdt == RT_NULL)
    {
        goto end;
    }

    num_mem_rsvmap = fdt_num_mem_rsv(fdt);
    off_mem_rsvmap = fdt_off_mem_rsvmap(fdt);
    rsvmap = (struct fdt_reserve_entry *)((char *)fdt + off_mem_rsvmap);

    for (i = 0; i < num_mem_rsvmap; ++i)
    {
        if (address == fdt64_to_cpu(rsvmap[i].address))
        {
            fdt_del_mem_rsv(fdt, i);
            break;
        }
    }

end:
    return fdt_totalsize(fdt);
}
