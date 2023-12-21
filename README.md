# Rootkit

### Générer un disk.img
```
./rootfs_builder/rootfs_builder.sh [path_to_linux-5.15.131]
```

### Compiler le rootkit
```
cd rootkit
make -C [path_to_linux-5.15.131] M=`pwd` modules
```

### Ajouter le rootkit.ko dans le disk.img
```
./scripts/add_kernel_object.sh rootfs_builder/buildfs/disk.img rootkit/rootkit.ko
```
