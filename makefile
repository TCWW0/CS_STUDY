# 定义目标和对象文件
object = csapp.o tiny.o
target = tiny

# 生成可执行文件
$(target) : $(object)
	g++ -o $(target) $(object) -lpthread

# 编译 csapp.o 和 tiny.o 需要依赖的头文件
csapp.o : csapp.h
	gcc -c csapp.c

tiny.o : tiny.c csapp.h
	gcc -c tiny.c

# 清理命令
.PHONY : clean
clean:
	rm -f $(target) $(object)
