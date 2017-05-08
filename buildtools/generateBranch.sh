#!/bin/bash

codeaddress=$1
branchaddress=$2
tempreg=$3

(cat > bl.s) << _EOF_
_start:
	lis $tempreg,0x$(echo $codeaddress | tr '[:upper:]' '[:lower:]' | cut -c3-6 -)
	ori $tempreg,$tempreg,0x$(echo $codeaddress | tr '[:upper:]' '[:lower:]' | cut -c7-10 -)
	mtctr $tempreg
	bctrl
_EOF_

/opt/devkitpro/devkitPPC/bin/powerpc-eabi-gcc -w -m32 -fno-exceptions -Os -nostdlib -nostartfiles -Ttext=$branchaddress -o bl.elf bl.s 2>/dev/null

/opt/devkitpro/devkitPPC/bin/powerpc-eabi-objdump -d bl.elf\
 | tail --lines=+8\
 | cut -c10-21 -\
 | tr -d " \n\t\r"\
 | xxd -r -p > branch.bin
