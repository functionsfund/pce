/*****************************************************************************
 * pce                                                                       *
 *****************************************************************************/

/*****************************************************************************
 * File name:     src/arch/sim405/pci.h                                      *
 * Created:       2004-11-16 by Hampa Hug <hampa@hampa.ch>                   *
 * Last modified: 2004-12-10 by Hampa Hug <hampa@hampa.ch>                   *
 * Copyright:     (C) 2004 Hampa Hug <hampa@hampa.ch>                        *
 *****************************************************************************/

/*****************************************************************************
 * This program is free software. You can redistribute it and / or modify it *
 * under the terms of the GNU General Public License version 2 as  published *
 * by the Free Software Foundation.                                          *
 *                                                                           *
 * This program is distributed in the hope  that  it  will  be  useful,  but *
 * WITHOUT  ANY   WARRANTY,   without   even   the   implied   warranty   of *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU  General *
 * Public License for more details.                                          *
 *****************************************************************************/

/* $Id$ */


#ifndef PCE_SIM405_PCI_H
#define PCE_SIM405_PCI_H 1


#define PCIID_INTEL_IXP2400 0x9001
#define PCIID_INTEL_IXP2800 0x9004


typedef struct {
  memory_t  asio;

  mem_blk_t pci_ioa;
  mem_blk_t pci_iob;
  mem_blk_t pci_cfg;
  mem_blk_t pci_mem;
  mem_blk_t pci_special;
  mem_blk_t pci_csr;

  pci_bus_t bus;
} pci_405_t;


void s405_pci_init (pci_405_t *pci);
void s405_pci_free (pci_405_t *pci);

pci_405_t *s405_pci_new (void);
void s405_pci_del (pci_405_t *pci);

mem_blk_t *s405_pci_get_mem_ioa (pci_405_t *pci);
mem_blk_t *s405_pci_get_mem_iob (pci_405_t *pci);
mem_blk_t *s405_pci_get_mem_cfg (pci_405_t *pci);
mem_blk_t *s405_pci_get_mem_special (pci_405_t *pci);
mem_blk_t *s405_pci_get_mem_csr (pci_405_t *pci);


#endif