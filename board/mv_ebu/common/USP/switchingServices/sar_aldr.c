/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	    this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
		notice, this list of conditions and the following disclaimer in the
		documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
		used to endorse or promote products derived from this software without
		specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include "sar_sw_lib.h"

/* help descriptions */
static char *h_devid = "Device_ID\n",
	    *h_devnum = "Device_Num\n"
			"For cascade system this is own device. For SMI this\n"
			"is PHY_Addr\n"
			"\tDeviceNum[4:2] is 3'b000\n"
			"\tDeviceNum[1:0] is sampled at reset\n",
	    *h_pll = "PLL0 Config\n"
		     "\t0 = 365MHz (121Gbps)\n"
		     "\t1 = Reserved\n"
		     "\t2 = 250MHz (84Gbps)\n"
		     "\t3 = 200MHz (67Gbps)\n"
		     "\t4 = 480MHz (84Gbps)\n"
		     "\t5 = Reserved\n"
		     "\t6 = Reserved\n"
		     "\t7 = PLL bypass (Reserved)\n",
	    *h_avs = "AVS mode\n"
		     "\t0 = slave mode\n"
		     "\t1 = master mode\n",
	    *h_cpuen = "CPU Enable\n"
		       "\t0 = Disable reading the EEPROM before CPU boot\n"
		       "\t1 = Enable reading the EEPROM before CPU boot\n",
	    *h_refclk = "IHB RefClk mode\n"
			"\t0 = Slave\n"
			"\t1 = Master\n",
	    *h_pcimod = "PCIe mode\n"
			"\t0 = PCI-E is Endpoint\n"
			"\t1 = PCI-E is Root-Complex\n",
	    *h_pciclk = "PCIe Clk mode\n"
			"\t0 = Internally generated by PLL3 MISC-PLL\n"
			"\t1 = External 100MHz input from PEX_CLK_P/N\n",
	    *h_i2caddr = "CPU_SMI_I2C_ADDR[2]\n"
			 "Used for: EEPROM I2C Address, Slave I2C Address\n"
			 "SMI slave address (all in Runit) Bits[1:0] are\n"
			 "taken from DEV_NUMBER[1:0]\n",
	    *h_pciself = "PCIE_SELF_INIT\n",
	    *h_devinit = "DEV_INIT_DONE\n",
	    *h_serdes1 = "Serdes1\n"
			 "There are 8 Serdes that can be configured to SFP\n"
			 "or to XG Module Card:\n"
			 "\t*For configuring SERDES [7:0] to SFP connectors\n"
			 "\t need to reset bit[7:0]\n"
			 "\t*For configuring SERDES [7:0] to XG module card\n"
			 "\t need to set bit[7:0]\n",
	    *h_serdes2 = "Serdes2\n"
			 "The CPU port Serdes (SERDES[32] can be configured\n"
			 "to rear SFP connector for supporting 1G/2.5G or to\n"
			 "AMC SGMII interface:\n"
			 "\t0 - For configuring CPU port to AMC\n"
			 "\t1 - For configuring CPU port to rear panel SFP\n"
			 "\t    connectors\n",
	    *h_boardid = "BoardID\n"
			 "\t0 - Board is DB-ALD-32XG\n"
			 "\t1 - Board is RD-ALD-24XNG-2XLG-A\n";

/* PCA9560PW	is used for all SatRs configurations (0x4c, 0x4d, 0x4f, 0x4e)
 * PCA9555	is used for all Serdes configurations (0x20)
 */
struct satr_info aldrin_satr_info[] = {
/*	name	twsi_addr  twsi_reg  field_of bit_mask moreThen256  default	help		pca9555*/
	{"devid",	0x4c,	0,	0,	0x1f,	MV_FALSE,	0x1f,	&h_devid,	MV_FALSE},
	{"devnum",	0x4d,	0,	0,	0x3,	MV_FALSE,	0x3,	&h_devnum,	MV_FALSE},
	{"corepll0",	0x4d,	0,	2,	0x7,	MV_FALSE,	0x4,	&h_pll,		MV_FALSE},
	{"avs",		0x4e,	0,	0,	0x1,	MV_FALSE,	0x1,	&h_avs,		MV_FALSE},
	{"cpueepromenable",	0x4e,	0,	1,	0x1,	MV_FALSE,	0x0,	&h_cpuen,	MV_FALSE},
	{"refclk",	0x4e,	0,	2,	0x1,	MV_FALSE,	0x1,	&h_refclk,	MV_FALSE},
	{"pciemode",	0x4e,	0,	3,	0x1,	MV_FALSE,	0x0,	&h_pcimod,	MV_FALSE},
	{"pcieclk",	0x4e,	0,	4,	0x1,	MV_FALSE,	0x1,	&h_pciclk,	MV_FALSE},
	{"cpui2caddr",	0x4f,	0,	0,	0x1,	MV_FALSE,	0x0,	&h_i2caddr,	MV_FALSE},
	{"pcie-self-init", 0x4f,0,	1,	0x1,	MV_FALSE,	0x1,	&h_pciself,	MV_FALSE},
	{"por-bypass-in", 0x4f,	0,	2,	0x1,	MV_FALSE,	0x1,	&h_devinit,	MV_FALSE},
	{"serdes1",	0x20,	2,	0,	0xFF,	MV_FALSE,	0xFF,	&h_serdes1,	MV_TRUE},
	{"serdes2",	0x20,	3,	0,	0x1,	MV_FALSE,	0x1,	&h_serdes2,	MV_TRUE},
	{"boardid",	0x53,	7,	0,	0x7,	MV_TRUE,	0x0,	&h_boardid,	MV_FALSE},
	/* the "LAST entry should be always last - it is used for SatR max options calculation */
	{"LAST",	0x0,	0,	0,	0x0,	MV_FALSE,	0x0,	NULL,		MV_FALSE},
};