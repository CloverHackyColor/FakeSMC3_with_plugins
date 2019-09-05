/*
 * Intel ACPI Component Architecture
 * AML/ASL+ Disassembler version 20160729-64
 * Copyright (c) 2000 - 2016 Intel Corporation
 *
 * Disassembling to symbolic ASL+ operators
 *
 * Disassembly of /Volumes/MacHD/Applications/0App/DarwinDumper_3.0.4_13.01_20.19.19_MacBookPro10,1_AMI_X64_4837_High Sierra_17G4015_sergey/ACPI Tables/AML/SSDT-2.aml, Sun Jan 13 20:23:34 2019
 *
 * Original Table Header:
 *     Signature        "SSDT"
 *     Length           0x0000016C (364)
 *     Revision         0x02
 *     Checksum         0xAE
 *     OEM ID           "APPLE "
 *     OEM Table ID     "Monitor"
 *     OEM Revision     0x00001000 (4096)
 *     Compiler ID      "INTL"
 *     Compiler Version 0x20160729 (538314537)
 */
DefinitionBlock ("", "SSDT", 2, "APPLE ", "Monitor", 0x00001000)
{
    External (_SB_.ADP1._PSR, MethodObj)    // 0 Arguments
    External (_SB_.LID0, DeviceObj)
    External (_SB_.LID0._LID, MethodObj)    // 0 Arguments
    External (_SB_.PCI0.LPCB.EC, DeviceObj)
    External (_TZ_.THM_._TMP, MethodObj)    // 0 Arguments

    Scope (\_SB.PCI0.LPCB.EC)
    {
        Device (FSAM)
        {
            Name (_HID, EisaId ("APP0111"))  // _HID: Hardware ID
            Name (_CID, "monitor")  // _CID: Compatible ID
            Name (PLID, 0xFFFF) //save previous LID status
            Name (PPSR, 0xFFFF)
            Name (BPSR, 0xFFFF)
            Method (MSLD, 0, NotSerialized)
            {
                Local0 = \_SB.LID0._LID ()
                If (Local0 != PLID)
                {
                    PLID = Local0
                    Notify (\_SB.LID0, 0x80) // Status Change
                }

                Return (Local0 ^ One)
            }

            Method (TSYS, 0, NotSerialized)
            {
                Local1 = MSLD ()
                Local0 = \_TZ.THM._TMP ()
                Local0 &= 0x7F
                Return (Local0)
            }

            Method (ACDC, 0, NotSerialized)
            {
                Local0 = \_SB.ADP1._PSR ()
                If (Local0 != PPSR)
                {
                    PPSR = Local0
                    BPSR = (PPSR ^ One)
                }

                Return (PPSR) /* \_SB_.PCI0.LPCB.EC.FSAM.PPSR */
            }

 //           Method (BATP, 0, NotSerialized)
 //           {
 //               Return (BPSR) /* \_SB_.PCI0.LPCB.EC.FSAM.BPSR */
 //           }
        }
    }
}

