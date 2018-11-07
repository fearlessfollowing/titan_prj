import os

list_dirs = os.walk("./")
for root, dirs, files in list_dirs:
    print(root)
    print(dirs)
    print(files)