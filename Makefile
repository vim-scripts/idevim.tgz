# Hack for the sort problem
export LC_ALL := c

CC    := gcc
CFLAGS := -g
LDBIN := ld
VIMBASE := /usr/share/vim/
INSTALL := /usr/bin/install


all: gdbvserv gdbvcl tags  install

common.o: common.c 
	$(CC) $(CFLAGS) -c common.c

gdbvserv.o: gdbvserv.c
	$(CC) $(CFLAGS) -c gdbvserv.c

gdbvcl.o: gdbvcl.c
	$(CC) $(CFLAGS) -c gdbvcl.c


gdbvserv: gdbvserv.o common.o
	$(CC) $(CFLAGS) gdbvserv.o common.o -o gdbvserv

ifneq ("$(VIM_SRC_BASE)", "")
INC_FLAGS := -I/usr/include/gtk-1.2 -I/usr/include/glib-1.2 -I/usr/lib/glib/include -I/usr/X11R6/include -I$(VIM_SRC_BASE)/src -I$(VIM_SRC_BASE)/src/proto/
gdbvcl_gtk.o: gdbvcl_gtk.c
	gcc -fpic -DFEAT_GUI_GTK $(INC_FLAGS) $(CFLAGS) -c -o gdbvcl_gtk.o gdbvcl_gtk.c
else
gdbvcl_gtk.o:; @echo
	@echo "please set the variable VIM_SRC_BASE to the path of the source directory"
	@echo "example: "
	@echo "		export VIM_SRC_BASE=/home/ligesh/vim60ax"
	@echo
	@false
endif

gtk:  gdbvcl_gtk.o common.o
	$(LDBIN) -o libvimgdb_gtk.so gdbvcl_gtk.o common.o -lc -shared ;


gdbvcl: gdbvcl.o common.o
	$(CC) $(CFLAGS) gdbvcl.o common.o -o gdbvcl
	$(LDBIN) -o libvimgdb.so gdbvcl.o common.o -lc -shared ;



tags: doctags
	./doctags ide.txt > tags.tmp
	uniq -u -2 tags.tmp tags 

doctags: doctags.c
	$(CC) doctags.c -o doctags


clean: 
	rm -f *.o *.so gdbvcl gdbvserv tags doctags tags.tmp core session.vim


install: installbin  installdoc
	

gtk_install: gtk
	$(INSTALL) libvimgdb_gtk.so /usr/lib

installbin: gdbvserv gdbvcl libvimgdb.so
	$(INSTALL) gdbvserv /usr/bin
	$(INSTALL) gdbvcl /usr/bin
	$(INSTALL) libvimgdb.so /usr/lib

ifneq ("$(VIM_VERSION)", "")
VIMDIR = $(VIMBASE)/vim$(VIM_VERSION)
installdoc: ide.txt tags ide.vim
	$(INSTALL) -m 0644 ide.txt $(VIMDIR)/doc/
	sort -o $(VIMDIR)/doc/tags $(VIMDIR)/doc/tags tags
	$(INSTALL) ide.vim  $(VIMDIR)/plugin/
else
installdoc:;@echo 
	@echo Please Set VIM_VERSION to the version of vim \(eg. 60\)
	@echo "	    Use 'vim --version' to find it.  See INSTALL"
	@echo
	@false
endif

vim_run:
	echo 
	
