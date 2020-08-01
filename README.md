How to use
-----------
Compile (Linux):
```
mkdir build
cd build
cmake ..
make -j
cd ..
```

After compiling:
First create a folder for the output and create a folder for the input.  Add all of the CGO/DGO files into the input folder.
```
build/jak_disassembler config/jak1_ntsc_black_label.jsonc in_folder/ out_folder/
```


Notes
--------
The `config` folder has settings for the disassembly. Currently Jak 2 and Jak 3 are not as well supported as Jak 1.
