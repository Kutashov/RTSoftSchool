lookat: main.o md5.o folder_tree.o
	gcc -o lookat main.o md5.o folder_tree.o -lm -lpthread -fsanitize=address 
cppcheck:
	cppcheck -j2 main.c main.h folder_tree.c folder_tree.h
checkpatch:
	checkpatch.pl --no-tree -f main.c main.h folder_tree.c folder_tree.h

main.o: main.c main.h
md5.o: md5.c md5.h
folder_tree.o: folder_tree.c folder_tree.h