#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	build
SOURCES		:=	source
DATA		:=	data  
INCLUDES	:=

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

#The address where the code will be injected in memory
CODEADDRESS = 0x80001800
#The address of the instruction that will be overwritten
#with a branch to adapter_thread()
#In this case, we're overwriting PAD_Read's call to SI_IsChanBusy,
#and adapter_thread will simulate this function accordingly :)
BRANCHADDRESS = 0x80216098

CFLAGS	= -mno-sdata -ffreestanding -mno-eabi -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables -s -Os -nostdlib --save-temps -nostartfiles -Wno-implicit-function-declaration -Wall $(MACHDEP) $(INCLUDE)
CXXFLAGS	=	$(CFLAGS)

LDFLAGS	=	$(MACHDEP) -Wl,-Map,$(notdir $@).map,-Ttext=$(CODEADDRESS)

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:=	

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) \
					$(sFILES:.s=.o) $(SFILES:.S=.o)

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:=	$(foreach dir,$(INCLUDES), -iquote $(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD) \
					-I$(LIBOGC_INC)

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
					#-L$(LIBOGC_LIB)

export OUTPUT	:=	$(CURDIR)/$(TARGET)
.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).dol $(OUTPUT).s $(OUTPUT).bin rii sd.raw gecko

#---------------------------------------------------------------------------------
sd.raw: rii
	@echo "Generating SD image..."
	@if [ -f "./sd.raw" ]; then rm ./sd.raw; fi
	@dd if=/dev/zero bs=1M count=256 of=./sd.raw
	@mkfs.fat -F 32 ./sd.raw
	@mcopy -s -i ./sd.raw ./rii/* ./gecko/* ::
	@echo "Done"

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------

#$(OUTPUT).dol: $(OUTPUT).elf

gecko: rii
	@echo "Creating gecko image..."
	@[ -d ../gecko ] || mkdir -p ../gecko
	@[ -d ../gecko/patch ] || mkdir -p ../gecko/patch
	@[ -d ../gecko/codes ] || mkdir -p ../gecko/codes
	@echo 0x01 | xxd -r -p > wii-gc-adapter.gpf
	@echo $(CODEADDRESS) | xxd -r -p >> wii-gc-adapter.gpf
	@echo $(shell printf "%08x\n" $(shell du -b $(OUTPUT).bin | cut -c1-4)) | xxd -r -p >> wii-gc-adapter.gpf
	@cat ../wii-gc-adapter-inject.bin >> wii-gc-adapter.gpf
	@cp wii-gc-adapter.gpf ../gecko/patch/RSBE01.gpf
	@../buildtools/createGCT
	@cp wii-gc-adapter.gct ../gecko/codes/RSBE01.gct
	@echo "Done"

rii: $(OUTPUT).bin
	@echo "Creating riivolution image..."
	@[ -d ../rii ] || mkdir -p ../rii
	@[ -d ../rii/riivolution ] || mkdir -p ../rii/riivolution
	@../buildtools/createRiiXML.sh $(CODEADDRESS) $(BRANCHADDRESS)
	@cp wii-gc-adapter.xml ../rii/riivolution
	@cp $< ../rii/riivolution/wii-gc-adapter-inject.bin
	@cp -r ../buildtools/apps ../rii
	@echo "Done"

$(OUTPUT).bin: $(OUTPUT).elf
	@powerpc-eabi-objcopy -O binary $< $@

$(OUTPUT).elf: $(OUTPUT).g.elf
	@powerpc-eabi-strip -s -R .init -R .fini -R .comment -o $@ $<
	@mv $(OUTPUT).g.elf wii-gc-adapter.g.elf
	
$(OUTPUT).g.elf: $(OFILES)



#---------------------------------------------------------------------------------
# This rule links in binary data with the .jpg extension
#---------------------------------------------------------------------------------
%.jpg.o	:	%.jpg
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	$(bin2o)

-include $(DEPENDS)


#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
