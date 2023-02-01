# RT-Hypervisor
RT-Hypervisor: A real-time hypervisor for automotive embedded system

### 请使用 `add_fdt` 分支

![bf9832a0e445a0c7f5bae509aa95fc2](https://user-images.githubusercontent.com/18082255/215932583-5f6e4205-6807-448d-9d98-8884b24912aa.png)

### QEMU启动命令：
```
qemu-system-aarch64 -M virt,gic-version=3 -cpu cortex-a53 -machine virtualization=on -kernel **rtthread.elf** -nographic \
-device loader,file=**./qemu/rtthread-std-.bin**,addr=0x45000000 \
-device loader,file=**./dump.dtb**,addr=0x46000000
```
#### 注意文件路径（RT-Hypervisor/bsp/qemu-virt64-aarch64）和需要补充的文件
* `rtthread.elf`：编译出的虚拟化镜像文件
* `./qemu/rtthread-std-.bin`：`Guest OS`镜像文件，按需修改
* `./dump.dtb`: 系统设备树数据文件，按需修改

##### 关于设备树命令（需要修改设备树，可按照下面相关命令自行修改）
* QEMU导出设备树：`qemu-system-aarch64 -M virt,gic-version=3 -cpu cortex-a53 -machine virtualization=on,dumpdtb=dump.dtb`
* dtb反编译生成dts：`dtc -I dtb -O dts -o dump.dts dump.dtb`
* dts编译生成dtb：`dtc -I dts -O dtb -o dump.dtb dump.dts`
