CC=gcc
CPPC=g++
LINK=g++
SRCDIR=.
OBJDIR=obj
FLAGS=-g -Wall -std=c++11
LIBS=-pthread
OBJS=$(OBJDIR)/main.o $(OBJDIR)/test1.o $(OBJDIR)/test2.o
EXEC=test_shared_mutex
DATE=$(shell date +"%Y-%m-%d")

$(EXEC) : $(OBJS)
	$(LINK) $(OBJS) -o $(EXEC) $(FLAGS) $(LIBS)

$(OBJDIR)/main.o: main.cpp $(OBJDIR)/__setup_obj_dir
	$(CPPC) $(FLAGS) main.cpp -c -o $@

$(OBJDIR)/test1.o: test1.cpp $(OBJDIR)/__setup_obj_dir
	$(CPPC) $(FLAGS) test1.cpp -c -o $@

$(OBJDIR)/test2.o: test2.cpp $(OBJDIR)/__setup_obj_dir
	$(CPPC) $(FLAGS) test2.cpp -c -o $@

$(OBJDIR)/__setup_obj_dir :
	mkdir -p $(OBJDIR)
	touch $(OBJDIR)/__setup_obj_dir

.PHONY: clean bzip release

clean :
	rm -rf $(OBJDIR)/*.o
	rm -rf $(EXEC)

bzip :
	tar -cvf "$(DATE).$(EXEC).tar" $(SRCDIR)/*.cpp $(SRCDIR)/*.h Makefile
	bzip2 "$(DATE).$(EXEC).tar"

release : FLAGS +=-O3 -D_RELEASE
release : $(EXEC)
