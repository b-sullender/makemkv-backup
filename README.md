# makemkv-backup
Library and program to automate creating DVD/Blu-ray backups using MakeMKV.

# Build and install the project
```shell
mkdir build && cd build
cmake ..
make
sudo make install
```

# Run MakeMKV backup
```shell
./makemkv-auto --device=/dev/sr0 --path="/output/path"
```
