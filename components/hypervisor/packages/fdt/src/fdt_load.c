#include <rthw.h>
#include <unistd.h>
#include <fcntl.h>

#include "libfdt/libfdt.h"
#include "fdt.h"

extern rt_err_t fdt_exec_status;

rt_inline rt_bool_t fdt_check(void *fdt)
{
    return fdt_check_header(fdt) == 0 ? RT_TRUE : RT_FALSE;
}

void *fdt_load_from_fs(char *dtb_filename)
{
    void *fdt = RT_NULL;
    rt_size_t dtb_sz;
    int fd = -1;

    if (dtb_filename == RT_NULL)
    {
        fdt_exec_status = FDT_RET_GET_EMPTY;
        goto end;
    }

    fd = open(dtb_filename, O_RDONLY, 0);

    if (fd == -1)
    {
        rt_kprintf("File `%s' not found.\n", dtb_filename);
        fdt_exec_status = FDT_RET_GET_EMPTY;
        goto end;
    }

    dtb_sz = lseek(fd, 0, SEEK_END);

    if (dtb_sz > 0)
    {
        if ((fdt = (struct fdt_header *)rt_malloc(sizeof(rt_uint8_t) * dtb_sz)) == RT_NULL)
        {
            fdt_exec_status = FDT_RET_NO_MEMORY;
            goto end;
        }

        lseek(fd, 0, SEEK_SET);
        read(fd, fdt, sizeof(rt_uint8_t) * dtb_sz);

        if (fdt_check(fdt) == RT_FALSE)
        {
            rt_free(fdt);
        }
    }

end:
    if (fd != -1)
    {
        close(fd);
    }

    return fdt;
}

void *fdt_load_from_memory(void *dtb_ptr, rt_bool_t is_clone)
{
    void *fdt = RT_NULL;

    if (dtb_ptr == RT_NULL)
    {
        fdt_exec_status = FDT_RET_GET_EMPTY;
        goto end;
    }

    if (fdt_check(dtb_ptr) == RT_FALSE)
    {
        fdt_exec_status = FDT_RET_GET_EMPTY;
        fdt = RT_NULL;
        goto end;
    }

    if (is_clone)
    {
        rt_size_t dtb_sz = fdt_totalsize(dtb_ptr);
        if (dtb_sz > 0)
        {
            if ((fdt = rt_malloc(dtb_sz)) != RT_NULL)
            {
                rt_memcpy(fdt, dtb_ptr, dtb_sz);
            }
            else
            {
                fdt_exec_status = FDT_RET_NO_MEMORY;
            }
        }
    }
    else
    {
        fdt = dtb_ptr;
    }

end:
    return fdt;
}
