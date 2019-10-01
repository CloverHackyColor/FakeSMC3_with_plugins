# FakeSMC3_with_plugins
Driver for emulation SMC device with hardware sensors support

## HWSensors
### This is a Mac OS X Package
Working 10.6 to 10.14

### HWSensors branch based on FakeSMC-3.x

### The package includes:
* FakeSMC.kext version 3.x
- ACPIMonitor.kext for custom making ACPI methods to access to hardware
- VoodooBatterySMC for laptop battery monitoring
- CPU sensors:
    + IntelCPUMonitor
    + AmdCpuMonitor
- GPUSensors
    + RadeonMonitor  for ATI/AMD Radeon card (temperature only)
    + GeforceSensors for Nvidia card Fermi, Kepler, Maxwell, Pascal
    + NVClockX for Nvidia Geforce 7xxx, 8xxx, Tesla
    + X3100 for IntelX3100 (at GM950 chipset)
- LPC chip sensors, motherboard parameters like FAM, Voltages, temperatures
    + ITEIT87x  for chips ITE 87xx, 86xx, usually present on Gigabyte motherboards
    + W836x  for chips Winbond/Nuvoton 83xxx, NCT67xx, usually present on ASUS motherboards
    + F718x  for chips Fintek 
    + PC8739x for chip SMC
- SMI Monitor
    + monitor and control temperature and fans in Dell computers by using SMM technology
- Applications 
    + HWMonitorSMC for old computers
    + HWMonitorSMC2 for SandyBridge and up


### HWSensors Project (c) 2010 netkas, slice, usr-sse2, kozlek, navi, vector sigma and other contributors. All rights reserved. 
