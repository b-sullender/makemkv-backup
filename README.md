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
./makemkv-backup --device=/dev/sr0 --path="/output/path"
```

# Building Debian Package
If you wish to build a Debian package, follow these steps:

1. **Ensure the `rules` file is executable**:  
   ```bash
   sudo chmod +x debian/rules
   ```

2. **Build the package**:  
   - Without signing:  
     ```bash
     sudo dpkg-buildpackage -uc -us
     ```
   - With signing (replace `<KEY_ID>` with your GPG key ID):  
     ```bash
     sudo dpkg-buildpackage -k<KEY_ID>
     ```
     *To list your GPG keys, use `gpg --list-keys`.*

3. **Cleanup build files**:  
   After building the package, remove temporary files with:  
   ```bash
   sudo debclean
   ```
   *This ensures your workspace is clean and free of leftover build artifacts.*

### Notes:
- Ensure you use `sudo` for all commands.
- Your newly created package files will be in the parent directory.
