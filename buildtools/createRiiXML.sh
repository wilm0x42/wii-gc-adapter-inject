#!/bin/bash

codeaddress=$1
branchaddress=$2

adapter_thread_addr=$(powerpc-eabi-readelf -s ./wii-gc-adapter.g.elf | grep adapter_thread | cut -c9-16)
adapter_getType_addr=$(powerpc-eabi-readelf -s ./wii-gc-adapter.g.elf | grep adapter_getType | cut -c9-16)
adapter_getStatus_addr=$(powerpc-eabi-readelf -s ./wii-gc-adapter.g.elf | grep adapter_getStatus | cut -c9-16)
adapter_getResponse_addr=$(powerpc-eabi-readelf -s ./wii-gc-adapter.g.elf | grep adapter_getResponse | cut -c9-16)
adapter_isChanBusy_addr=$(powerpc-eabi-readelf -s ./wii-gc-adapter.g.elf | grep adapter_isChanBusy | cut -c9-16)
adapter_controlMotor_addr=$(powerpc-eabi-readelf -s ./wii-gc-adapter.g.elf | grep adapter_controlMotor | cut -c9-16)
start_addr=$(powerpc-eabi-readelf -h ./wii-gc-adapter.g.elf | grep "Entry point address:" | cut -c38- | tr -d '\n\r')

echo "Place code in memory at $codeaddress."
echo "Overwrite Branch Address ($branchaddress) with"
echo "code that will branch to adapter_thread ($adapter_thread_addr)"
echo "Initially branch to code at $start_addr from"
echo "0x802288c4 (USB_LOG), with one-time exec code."
echo "Overwrite PADRead's call to SI_GetType (8021619c)"
echo "with a call to adapter_getType ($adapter_getType_addr)."
echo "Overwrite PADRead's call to SI_GetStatus (802160c0)"
echo "with a call to adapter_getStatus ($adapter_getStatus_addr)."
echo "Overwrite PADRead's call to SI_GetResponse (802161b4)"
echo "with a call to adapter_getResponse ($adapter_getResponse_addr)."
echo "Overwrite PADRead's call to SI_IsChanBusy (80216098)"
echo "with a call to adapter_isChanBusy ($adapter_isChanBusy_addr)."
echo "Redirect PAD_ControlMotor (802162c4) to"
echo "adapter_controlMotor ($adapter_controlMotor_addr)."
echo "Overwrite PADUpdateOrigin's call to SI_GetType (8021556c)"
echo "with a call to adapter_getType ($adapter_getType_addr)."

#Branch to _start from within USB_LOG
start_bl=$(../buildtools/generateBl 0x802288c4 $start_addr)

#Branch to adapter_thread
adapter_thread_bl=$(../buildtools/generateBl $branchaddress 0x$adapter_thread_addr)

#Function overwrites
adapter_getType_bl=$(../buildtools/generateBl 0x8021619c 0x$adapter_getType_addr)
adapter_getStatus_bl=$(../buildtools/generateBl 0x802160c0 0x$adapter_getStatus_addr)
adapter_getResponse_bl=$(../buildtools/generateBl 0x802161b4 0x$adapter_getResponse_addr)
adapter_isChanBusy_bl=$(../buildtools/generateBl 0x80216098 0x$adapter_isChanBusy_addr)
adapter_controlMotor_bl=$(../buildtools/generateBl 0x802162c4 0x$adapter_controlMotor_addr)
adapter_getType_bl2=$(../buildtools/generateBl 0x8021556c 0x$adapter_getType_addr)


(cat > wii-gc-adapter.xml) << _EOF_
<wiidisc version="1" root="/riivolution">
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
      <!-- Branch -->
      <memory offset="$branchaddress" value="0x$adapter_thread_bl" />
      
      <!-- Substitute call to SI_GetType with adapter_getType -->
      <memory offset="0x8021619c" value="0x$adapter_getType_bl" />
      <!-- Subsittute call to SI_GetStatus wtih adapter_getStatus -->
      <memory offset="0x802160c0" value="0x$adapter_getStatus_bl" />
      <!-- Substitute call to SI_GetResponse with adapter_getResponse -->
      <memory offset="0x802161b4" value="0x$adapter_getResponse_bl" />
      <!-- Substitute call to SI_IsChanBusy with adapter_isChanBusy -->
      <memory offset="0x80216098" value="0x$adapter_isChanBusy_bl" />
      
      <!-- Redirect PAD_ControlMotor to adapter_controlMotor -->
      <memory offset="0x802162c4" value="0x$adapter_controlMotor_bl" />
      <memory offset="0x802162c8" value="0x48000078" />
      
      <!-- Substitute PAD_UpdateOrigin's call to SI_GetType with adapter_getType -->
      <memory offset="0x8021556c" value="0x$adapter_getType_bl2" />

      <!-- Make USB_LOG run -->
      <memory offset="0x80228874" value="0x2c000001" />
      <!-- Branch to _start from within USB_LOG -->
      <memory offset="0x802288c4" value="0x$start_bl" />
      <!-- nop early calls to USB_LOG (orig usb heap not initialized yet) -->
      <memory offset="0x80228a00" value="0x60000000" />
      <memory offset="0x802289c8" value="0x60000000" />
      
      <!-- Spoof gamecube controller ports -->
      <memory offset="0x8021601c" value="0x41820078" />
   </patch>
</wiidisc>
_EOF_