
###
# Test if root directory environment variable is defined.
###
ifeq "$(strip $(ROOT_DIR))" ""
$(error "Please define the ROOT_DIR environment variable to point to project root dir")
endif



###
# Compiling C files with this one
###
SRC_SUFFIX=c

CC:=gcc

###
# This is an obscure exercise: 
# Each target gets defined again, but in the way of doing
# so we copy its name into the OP global variable, when we have to recurse into a subdirectory, then
# the recursion rule knows, what the current target is, by looking at the OP variable, and so
# the current build target gets passed to the invoked makefile that sits in the subdirectory.
###
clean:OP:=clean
cleanall:OP:=cleanall
dep:OP:=dep
check:OP:=check


##########################################################################################
# Compiler flags: general settings
##########################################################################################

ifndef NO_STRICT
CFLAGS:=-W -Wall -Werror -Wpointer-arith -pipe 
endif


##########################################################################################
# Compiler flags: Debug information
##########################################################################################

ifndef NDEBUG
NDEBUG:=0
endif

ifeq ($(NDEBUG),0)
CFLAGS+= -g -D_DEBUG_
else
CFLAGS+= -DNDEBUG
endif

##########################################################################################
# Compiler flags: Optimization flags
##########################################################################################

ifdef OPT
ifneq ($(OPT),0)
OPTIMIZE:=1
endif
endif

ifdef OPTIMIZE
CFLAGS+= -O3 -ffast-math -finline-functions -funroll-all-loops -finline-limit=800 -mmmx -msse -foptimize-sibling-calls

ifdef PG
NO_OMIT_FRAME_POINTER=1
endif

ifndef NO_OMIT_FRAME_POINTER
CFLAGS+= -fomit-frame-pointer
endif

endif

##########################################################################################
# Compiler flags: Additional flags.
##########################################################################################

ifneq "$(strip $(ADD_FLAGS))" ""
CFLAGS += $(ADD_FLAGS)
endif

ifneq "$(strip $(ADD_CXX_FLAGS))" ""
CXXFLAGS += $(ADD_CXX_FLAGS)
endif



##########################################################################################
# Compiler flags: Libraries
##########################################################################################

# profiling support #

ifdef PG
CFLAGS+= -pg
LDFLAGS+= -pg
endif

### c runtime library
ifndef NO_LIBSTDC
LDFLAGS+= -lc
endif

### dynamic loading
ifndef NO_LIBDL
LDFLAGS+= -ldl
endif

### thread package
ifndef NO_LIBTHREAD
LDFLAGS+=-lpthread
endif

ifndef NO_LIBRT
LDFLAGS+=-lrt
endif

### user defined list of libraries  (var set in including script)
ifneq "$(strip $(ADD_LIBRARIES))" ""
LDFLAGS += -L$(ROOT_DIR)/linux/lib $(ADD_LIBRARIES)
endif

##########################################################################################
# Compiler flags: Include path
##########################################################################################

### user defined list of includes (var set in including script)
ifneq "$(strip $(ADD_INCLUDES))" ""
CFLAGS += $(ADD_INCLUDES)
endif


##########################################################################################
# Implicit build rules
# We want to put in all objects into subdirectories.
##########################################################################################

CURWD:=$(shell pwd)
OBJ_FILE_DIR:=$(ROOT_DIR)/linux/obj/$(TARGET_NAME)$(CURWD)

$(OBJ_FILE_DIR)/%.o: %.c
	$(COMPILE.c) $(OUTPUT_OPTION) $(CURWD)/$<

##########################################################################################
# Set generic target name variable
##########################################################################################

ifneq "$(strip $(TARGET_EXE))" ""

export TARGET_NAME:=$(TARGET_EXE)

else
ifneq "$(strip $(TARGET_SOLIB))" ""

export TARGET_NAME:=$(TARGET_SOLIB)

else
ifneq "$(strip $(TARGET_LIB))" "" 

export TARGET_NAME:=$(TARGET_LIB)

endif
endif
endif

TARGET_POST_BUILD_SCRIPT:=$(strip $(basename $(TARGET_NAME))).sh



##########################################################################################
# Main Build Targets
##########################################################################################



###
# Check: do we want to create an executable target ??
###
ifneq "$(strip $(TARGET_EXE))" ""

###
# EXE target: goto all subdirectories and gather the obj files
###



#src_filez=$(shell find $(CURWD) -name '*.$(SRC_SUFFIX)')

objects:=$(foreach dir,$(SUBDIRS),$(patsubst %.$(SRC_SUFFIX),$(OBJ_FILE_DIR)/$(basename %).o,$(wildcard $(dir)/*.$(SRC_SUFFIX))))

#objects:=$(foreach dir,$(SUBDIRS),$(patsubst %.$(SRC_SUFFIX),$(OBJ_FILE_DIR)/$(basename %).o,$(src_filez)))


#objects:=$(foreach dir,$(SUBDIRS),$(wildcard $(dir)/*.$(SRC_SUFFIX)))

#objects:=$(foreach dir,$(SUBDIRS),$(dir))



.PHONY: main
main : obj_dir subdirs current_dir $(TARGET_EXE) 

$(TARGET_EXE): $(objects)
	$(CC) -o $(TARGET_EXE) $(objects) $(LDFLAGS)
	@cp -f $(TARGET_EXE) $(ROOT_DIR)/linux/bin
	@set -e; if [ -x ./$(TARGET_POST_BUILD_SCRIPT) ]; then ./$(TARGET_POST_BUILD_SCRIPT); fi

### **** eof executable target rules ****

else

###
# Check: do we want to build a shared library target ??
###
ifneq "$(strip $(TARGET_SOLIB))" ""

###
# Shared library target: goto all subdirectories and gather the obj files
###
objects:=$(foreach dir,$(SUBDIRS),$(patsubst %.$(SRC_SUFFIX),$(OBJ_FILE_DIR)/$(basename %).o,$(wildcard $(dir)/*.$(SRC_SUFFIX))))

.PHONY: main
main : obj_dir subdirs current_dir $(TARGET_SOLIB)


$(TARGET_SOLIB): $(objects)
	@echo "Linking shared library"
ifeq "$(strip $(NO_LIBSTDC))" ""
	$(LD) -shared -Wl,-soname,$(TARGET_SOLIB) -o $(TARGET_SOLIB) $(objects) $(LDFLAGS)
else
	$(LD) -shared -o $(TARGET_SOLIB) $(objects) $(LDFLAGS)
endif
	@cp -f $(TARGET_SOLIB) $(ROOT_DIR)/build/lib
	@cp -f $(TARGET_SOLIB) $(ROOT_DIR)/lib/ 
	@set -e; if [ -x ./$(TARGET_POST_BUILD_SCRIPT) ]; then ./$(TARGET_POST_BUILD_SCRIPT); fi
 

### **** eof shared library target rules ****

else

###
# Check: do we want to create a static library target.
###
ifneq "$(strip $(TARGET_LIB))" "" 

###
# We want to generate a target, goto all subdirectories and gather the obj files
###
objects:=$(foreach dir,$(SUBDIRS),$(patsubst %.$(SRC_SUFFIX),$(OBJ_FILE_DIR)/$(basename %).o,$(wildcard $(dir)/*.$(SRC_SUFFIX))))



.PHONY: main
main: obj_dir subdirs current_dir $(TARGET_LIB)

.PHONY: $(TARGET_LIB) 
$(TARGET_LIB): $(objects) 
	$(RM) $@
	$(AR) $(ARFLAGS) $@ $(objects)
	@cp -f $@ $(ROOT_DIR)/linux/lib/
	@set -e; if [ -x ./$(TARGET_POST_BUILD_SCRIPT) ]; then ./$(TARGET_POST_BUILD_SCRIPT); fi
 

### **** eof static library target rules ****

else

###
# Check: do we want to create a unit test.
###
ifneq "$(strip $(TEST_PACKAGES))" "" 

###
# Find all objects from stuff that we want to include
###
package_objects:=$(foreach dir,$(TEST_PACKAGES),$(patsubst %.$(SRC_SUFFIX),$(OBJ_FILE_DIR)/%.o,$(wildcard $(TOPDIR)/$(dir)/*.$(SRC_SUFFIX))))

###
# Find the unit test files.
###
test_objs:=$(patsubst %.$(SRC_SUFFIX),%.o,$(wildcard *.$(SRC_SUFFIX)))

###
# Add cppunit includes and libraries.
###
#CLAGS+=-I $(ROOT_OSS_BUILD)/include/cppunit
#LDFLAGS+=-lcppunit

.PHONY: main
main: obj_dir $(test_objs) $(package_objects) 
	$(LD) -o unit_test $(test_objs) $(package_objects)  $(LDFLAGS)

### **** eof unit test target rules ****

else

###
# No target - compile sources in current directory.
###

.PHONY: main
main: obj_dir subdirs current_dir

endif
endif
endif
endif


###
# object file directory.
###
.PHONY: obj_dir
obj_dir:
	@mkdir -p $(OBJ_FILE_DIR)



##
#if a list of subdirectories is defined, put in a rule that goes into them 
##
ifneq "$(strip $(SUBDIRS))" ""

.PHONY: subdirs
subdirs: $(SUBDIRS)


.PHONY: $(SUBDIRS)
$(SUBDIRS):
	@if [ "x$@" != "x." ]; then  $(MAKE) -C $@ $(OP) ; fi

else

.PHONY: subdirs
subdirs:


endif



#
# current_dir - target to make files in current directory.
# Find list of objects in current directory and compiles them.
###
source_here:=$(wildcard *.$(SRC_SUFFIX)) 
objects_here:=$(strip $(patsubst %.$(SRC_SUFFIX),$(OBJ_FILE_DIR)/%.o,$(source_here)))

.PHONY: current_dir
current_dir : $(objects_here)	


##########################################################################################
# Clean Target
##########################################################################################

cleanall: clean
	@find $(TOPDIR) -name $(DEPEND) | xargs rm -f	

clean: subdirs clean_target
	@find $(TOPDIR) -name unit_test | xargs rm -f
	@find $(OBJ_FILE_DIR) -name \*.o | xargs rm -f

ifneq "$(strip $(TARGET_EXE))" ""

.PHONY: clean_target
clean_target:
	@rm -f $(TARGET_EXE)
	@rm -f $(ROOT_DIR)/linux/bin/$(TARGET_EXE) 

else

ifneq "$(strip $(TARGET_LIB))" "" 

.PHONY: clean_target
clean_target:
	@rm -f $(TARGET_LIB)
	@rm -f $(ROOT_DIR)/linux/lib/$(TARGET_LIB) 
	@rm -f $(ROOT_DIR)/linux/lib/$(basename $(TARGET_LIB))


else

ifneq "$(strip $(TARGET_SOLIB))" "" 

.PHONY: clean_target
clean_target:
	@rm -f $(TARGET_SOLIB)
	@rm -f $(ROOT_DIR)/linux/lib/$(TARGET_SOLIB) 


else

.PHONY: clean_target
clean_target:


endif

endif

endif


 

