#!/bin/bash

codeaddress=$1
branchaddress=$2

adapter_thread_addr=$(powerpc-eabi-readelf -s ./wii-gc-adapter.g.elf | grep adapter_thread | cut -c9-16)
adapter_getType_addr=$(powerpc-eabi-readelf -s ./wii-gc-adapter.g.elf | grep adapter_getType | cut -c9-16)
start_addr=$(powerpc-eabi-readelf -h ./wii-gc-adapter.g.elf | grep "Entry point address:" | cut -c38- | tr -d '\n\r')

echo "Place code in memory at $codeaddress."
echo "Overwrite Branch Address ($branchaddress) with"
echo "code that will branch to adapter_thread ($adapter_thread_addr)"
echo "Initially branch to code at $start_addr from"
echo "0x802288bc (USB_LOG), with one-time exec code."
echo "Overwrite PADRead's call to SI_GetType (8021619c)"
echo "with a call to adapter_getType ($adapter_getType_addr)"

#Branch to _start from within USB_LOG
start_bl=$(../buildtools/generateBl 0x802288bc $start_addr)

#Branch to adapter_thread from PADRead
adapter_thread_bl=$(../buildtools/generateBl $branchaddress 0x$adapter_thread_addr)

adapter_getType_bl=$(../buildtools/generateBl 0x8021619c 0x$adapter_getType_addr)


(cat > wii-gc-adapter.xml) << _EOF_
<wiidisc version="1">
   <id game="RSB">
       <region type="E" />
   </id>

   <options>
      <section name="wii-gc-adapter">
         <option id="enable" name="Enable wii-gc-adapter" default="1">
            <choice name="Enabled">
               <patch id="patch" />
            </choice>
         </option>
      </section>
   </options>
   
   <patch id="patch">
      <!-- Actual code -->
      <memory offset="$codeaddress" valuefile="/wii-gc-adapter-inject.bin" />
      
      <!-- Substitute call to SI_GetType with adapter_getType -->
      <memory offset="0x8021619c" value="0x$adapter_getType_bl" />
      <!-- Branch to adapter_thread from PADRead -->
      <memory offset="$branchaddress" value="0x$adapter_thread_bl" />
      
      
      <!-- cmpwi r0, 1 -->
      <memory offset="0x80228874" value="0x2c000001" />
      <!-- Branch to _start from within USB_LOG -->
      <memory offset="0x802288bc" value="0x$start_bl" />
      <!-- li r0, 1 -->
      <memory offset="0x802288c0" value="0x38000001" />
      <!-- stb r0, -0x3748, (r13) (Disable USB_LOG) -->
      <memory offset="0x802288c4" value="0x980dc8b8" />
      
      <!-- nop the first call to USB_LOG (USB heap not initialized at this point) -->
      <memory offset="0x80228a00" value="0x60000000" />
      <!-- Increase USB heap size 0x4000 -> 0x6000 -->
      <memory offset="0x80228a34" value="0x38806000" />
      

      <!-- Spoof gamecube controller port 1 (Thx spunit262) -->
      <memory offset="0x806971CC" value="0x60000000" />
      <memory offset="0x8152e29c" value="0x00000000" />
   </patch>
</wiidisc>
_EOF_