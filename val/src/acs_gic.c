/** @file
 * Copyright (c) 2016-2018, 2021 Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

#include "include/bsa_acs_val.h"
#include "include/bsa_acs_gic.h"
#include "include/bsa_acs_gic_support.h"
#include "include/bsa_acs_common.h"
#include "sys_arch_src/gic/gic.h"

GIC_INFO_TABLE  *g_gic_info_table;

/**
  @brief   This API executes all the GIC tests sequentially
           1. Caller       -  Application layer.
           2. Prerequisite -  val_gic_create_info_table()
  @param   num_pe - the number of PE to run these tests on.
  @param   g_sw_view - Keeps the information about which view tests to be run
  @return  Consolidated status of all the tests run.
**/
uint32_t
val_gic_execute_tests(uint32_t num_pe, uint32_t *g_sw_view)
{

  uint32_t status, i;
  uint32_t gic_version, num_msi_frame;

  for (i=0 ; i<MAX_TEST_SKIP_NUM ; i++){
      if (g_skip_test_num[i] == ACS_GIC_TEST_NUM_BASE) {
          val_print(ACS_PRINT_TEST, "\n       USER Override - Skipping all GIC tests \n", 0);
          return ACS_STATUS_SKIP;
      }
  }

  status      = ACS_STATUS_PASS;
  gic_version = val_gic_get_info(GIC_INFO_VERSION);

  if (g_sw_view[G_SW_OS]) {
      val_print(ACS_PRINT_ERR, "\nOperating System View:\n", 0);
      status |= os_g001_entry(num_pe);
      status |= os_g002_entry(num_pe);

      if (gic_version > 2) {
        status |= os_g003_entry(num_pe);
        status |= os_g004_entry(num_pe);
      }

      status |= os_g005_entry(num_pe);
      status |= os_g006_entry(num_pe);
  }

  if (g_sw_view[G_SW_HYP]) {
      val_print(ACS_PRINT_ERR, "\nHypervisor View:\n", 0);
      status |= hyp_g001_entry(num_pe);
  }

  /* Run GICv2m only if GIC Version is v2m. */
  num_msi_frame = val_gic_get_info(GIC_INFO_NUM_MSI_FRAME);

  if ((gic_version != 2) || (num_msi_frame == 0)) {
      val_print(ACS_PRINT_TEST, "\n       No GICv2m, Skipping all GICv2m tests \n", 0);
      goto its_test;
  }

  if (val_gic_v2m_parse_info()) {
      val_print(ACS_PRINT_TEST, "\n       GICv2m info mismatch, Skipping all GICv2m tests \n", 0);
      goto its_test;
  }

  val_print(ACS_PRINT_ERR, "\n      *** Starting GICv2m tests ***\n", 0);
  if (g_sw_view[G_SW_OS]) {
      val_print(ACS_PRINT_ERR, "\nOperating System View:\n", 0);
      status |= os_v2m001_entry(num_pe);
      status |= os_v2m002_entry(num_pe);
      status |= os_v2m003_entry(num_pe);
      status |= os_v2m004_entry(num_pe);
  }

its_test:
  if ((val_gic_get_info(GIC_INFO_NUM_ITS) == 0) || (pal_target_is_dt())) {
      val_print(ACS_PRINT_DEBUG, "\n       No ITS, Skipping all ITS tests \n", 0);
      goto test_done;
  }
  val_print(ACS_PRINT_ERR, "\n      *** Starting ITS tests ***\n", 0);
  if (g_sw_view[G_SW_OS]) {
      val_print(ACS_PRINT_ERR, "\nOperating System View:\n", 0);
      status |= os_its001_entry(num_pe);
      status |= os_its002_entry(num_pe);
      status |= os_its003_entry(num_pe);
      status |= os_its004_entry(num_pe);
  }

test_done:
  if (status != ACS_STATUS_PASS)
    val_print(ACS_PRINT_TEST, "\n      *** One or more tests have Failed/Skipped.*** \n", 0);
  else
    val_print(ACS_PRINT_TEST, "\n       All GIC tests Passed!! \n", 0);

  return status;

}


/**
  @brief   This API will call PAL layer to fill in the GIC information
           into the g_gic_info_table pointer.
           1. Caller       -  Application layer.
           2. Prerequisite -  Memory allocated and passed as argument.
  @param   gic_info_table  pre-allocated memory pointer for gic_info
  @return  Error if Input param is NULL
**/
uint32_t
val_gic_create_info_table(uint64_t *gic_info_table)
{

  if (gic_info_table == NULL) {
      val_print(ACS_PRINT_ERR, "Input for Create Info table cannot be NULL \n", 0);
      return ACS_STATUS_ERR;
  }
  val_print(ACS_PRINT_INFO, " Creating GIC INFO table\n", 0);

  g_gic_info_table = (GIC_INFO_TABLE *)gic_info_table;

  pal_gic_create_info_table(g_gic_info_table);

  val_print(ACS_PRINT_TEST, " GIC_INFO: Number of GICD             : %4d \n", g_gic_info_table->header.num_gicd);
  val_print(ACS_PRINT_TEST, " GIC_INFO: Number of ITS              : %4d \n", g_gic_info_table->header.num_its);

  if (g_gic_info_table->header.num_gicd == 0) {
      val_print(ACS_PRINT_ERR,"\n ** CRITICAL ERROR: GIC Distributor count is 0 **\n", 0);
      return ACS_STATUS_ERR;
  }

  if (pal_target_is_dt())
      val_bsa_gic_init();
  return ACS_STATUS_PASS;
}

/**
  @brief   This API frees the memory assigned for gic info table
           1. Caller       -  Application Layer
           2. Prerequisite -  val_gic_create_info_table
  @param   None
  @return  None
**/
void
val_gic_free_info_table(void)
{
  pal_mem_free((void *)g_gic_info_table);
}

/**
  @brief   This API returns the base address of the GIC Distributor.
           The assumption is we have only 1 GIC Distributor. IS this true?
           1. Caller       -  VAL
           2. Prerequisite -  val_gic_create_info_table
  @param   None
  @return  Address of GIC Distributor
**/
addr_t
val_get_gicd_base(void)
{

  GIC_INFO_ENTRY  *gic_entry;

  if (g_gic_info_table == NULL) {
      val_print(ACS_PRINT_ERR, "GIC INFO table not available \n", 0);
      return 0;
  }

  gic_entry = g_gic_info_table->gic_info;

  while (gic_entry->type != 0xFF) {
    if (gic_entry->type == ENTRY_TYPE_GICD) {
        return gic_entry->base;
    }
    gic_entry++;
  }

  return 0;
}

/**
  @brief   This API returns the base address of the GIC Redistributor for the current PE
           1. Caller       -  Test Suite
           2. Prerequisite -  val_gic_create_info_table
  @param   rdbase_len - To Store the Lenght of the Redistributor
  @return  Address of GIC Redistributor
**/
addr_t
val_get_gicr_base(uint32_t *rdbase_len)
{
  GIC_INFO_ENTRY  *gic_entry;

  if (g_gic_info_table == NULL) {
      val_print(ACS_PRINT_ERR, "GIC INFO table not available \n", 0);
      return 0;
  }

  gic_entry = g_gic_info_table->gic_info;

  while (gic_entry->type != 0xFF) {
      if (gic_entry->type == ENTRY_TYPE_GICR_GICRD) {
              *rdbase_len = gic_entry->length;
              return gic_entry->base;
      }
      if (gic_entry->type == ENTRY_TYPE_GICC_GICRD) {
              *rdbase_len = 0;
              return gic_entry->base;
      }
      gic_entry++;
  }

  *rdbase_len = 0;
  return 0;
}

/**
  @brief   This API returns the base address of the GICH.
           1. Caller       -  VAL
           2. Prerequisite -  val_gic_create_info_table
  @param   None
  @return  Address of GICH
**/
addr_t
val_get_gich_base(void)
{

  GIC_INFO_ENTRY  *gic_entry;

  if (g_gic_info_table == NULL) {
      val_print(ACS_PRINT_ERR, "GIC INFO table not available \n", 0);
      return 0;
  }

  gic_entry = g_gic_info_table->gic_info;

  while (gic_entry->type != 0xFF) {
    if (gic_entry->type == ENTRY_TYPE_GICH) {
        return gic_entry->base;
    }
    gic_entry++;
  }

  return 0;
}
/**
  @brief   This API returns the base address of the CPU IF for the current PE
           1. Caller       -  Test Suite
           2. Prerequisite -  val_gic_create_info_table
  @param   None
  @return  Address of GIC Redistributor
**/
addr_t
val_get_cpuif_base(void)
{
  GIC_INFO_ENTRY  *gic_entry;

  if (g_gic_info_table == NULL) {
      val_print(ACS_PRINT_ERR, "GIC INFO table not available \n", 0);
      return 0;
  }

  gic_entry = g_gic_info_table->gic_info;

  if (gic_entry) {
      while (gic_entry->type != 0xFF) {
          if (gic_entry->type == ENTRY_TYPE_CPUIF)
              return gic_entry->base;
          gic_entry++;
      }
  }

  return 0;
}

/**
  @brief   This function is a single point of entry to retrieve
           all GIC related information.
           1. Caller       -  Test Suite
           2. Prerequisite -  val_gic_create_info_table
  @param   type   the type of information being requested
  @return  32-bit data
**/
uint32_t
val_gic_get_info(uint32_t type)
{

  uint32_t rdbase_len;
  if (g_gic_info_table == NULL) {
      val_print(ACS_PRINT_ERR, "\n   Get GIC info called before gic info table is filled ",        0);
      return 0;
  }

  switch (type) {

      case GIC_INFO_VERSION:
          if (g_gic_info_table->header.gic_version != 0) {
             val_print(ACS_PRINT_INFO, "\n       gic version from info table = %d ",
                       g_gic_info_table->header.gic_version);
             return g_gic_info_table->header.gic_version;
          }
          /* Read Version from GICD_PIDR2 bit [7:4] */
          return VAL_EXTRACT_BITS(val_mmio_read(val_get_gicd_base() + GICD_PIDR2), 4, 7);

      case GIC_INFO_SEC_STATES:
          /* Read DS Bit from GICD_CTLR bit[6] */
          return VAL_EXTRACT_BITS(val_mmio_read(val_get_gicd_base() + GICD_CTLR), 6, 6);

      case GIC_INFO_AFFINITY_NS:
          /* Read ARE_NS Bit from GICD_CTLR bit[5] */
          return VAL_EXTRACT_BITS(val_mmio_read(val_get_gicd_base() + GICD_CTLR), 4, 4);

      case GIC_INFO_ENABLE_GROUP1NS:
          /* Read EnableGrp1NS Bit from GICD_CTLR bit[2] */
          return VAL_EXTRACT_BITS(val_mmio_read(val_get_gicd_base() + GICD_CTLR), 0, 1);

      case GIC_INFO_SGI_NON_SECURE:
          /* The non-RAZ/WI bits from GICR_ISENABLER0 correspond to non-secure SGIs */
          return val_mmio_read(val_get_gicr_base(&rdbase_len) + RD_FRAME_SIZE + GICR_ISENABLER);

      case GIC_INFO_SGI_NON_SECURE_LEGACY:
          /* The non-RAZ/WI bits from GICD_ISENABLER<n> correspond to non-secure SGIs */
          return val_mmio_read(val_get_gicd_base() + GICD_ISENABLER);

      case GIC_INFO_NUM_ITS:
          return g_gic_info_table->header.num_its;

      case GIC_INFO_NUM_MSI_FRAME:
          return g_gic_info_table->header.num_msi_frame;

      default:
          val_print(ACS_PRINT_ERR, "\n    GIC Info - TYPE not recognized %d  ", type);
          break;
  }
  return ACS_STATUS_ERR;
}

/**
  @brief   This API returns the max interrupt ID supported by the GIC Distributor
           1. Caller       -  VAL
           2. Prerequisite -  val_gic_create_info_table
  @param   None
  @return  Maximum Interrupt ID
**/
uint32_t
val_get_max_intid(void)
{
  return (32 * ((val_mmio_read(val_get_gicd_base() + GICD_TYPER) & 0x1F) + 1));
}

/**
  @brief   This function routes interrupt to specific PE.
           1. Caller       -  Test Suite
           2. Prerequisite -  val_gic_create_info_table
  @param   int_id Interrupt ID to be routed
  @param   mpidr MPIDR_EL1 reg value of the PE to which the interrupt should be routed
  @return  status
**/
uint32_t val_gic_route_interrupt_to_pe(uint32_t int_id, uint64_t mpidr)
{
  uint64_t cpuaffinity;

  if (int_id > 31) {
      cpuaffinity = mpidr & (PE_AFF0 | PE_AFF1 | PE_AFF2 | PE_AFF3);
      val_mmio_write64(val_get_gicd_base() + GICD_IROUTER + (8 * int_id), cpuaffinity);
  }
  else{
      val_print(ACS_PRINT_ERR, "\n    Only SPIs can be routed, interrupt with INTID = %d cannot be routed", int_id);
  }

  return 0;
}

/**
  @brief   This function will return '1' if an interrupt is either pending or active.
           1. Caller       -  Test Suite
           2. Prerequisite -  val_gic_create_info_table
  @param   int_id Interrupt ID
  @return  pending/active status
**/
uint32_t val_gic_get_interrupt_state(uint32_t int_id)
{
  uint32_t reg_offset = int_id / 32;
  uint32_t reg_shift  = int_id % 32;
  uint32_t mask = (1 << reg_shift);
  uint32_t active, pending;

  pending = val_mmio_read(val_get_gicd_base() + GICD_ISPENDR + (4 * reg_offset));
  active = val_mmio_read(val_get_gicd_base() + GICD_ISACTIVER0 + (4 * reg_offset));

  return ((mask & active) || (mask & pending));
}

/**
  @brief   This function will clear an interrupt that is pending or active.
           1. Caller       -  Test Suite
           2. Prerequisite -  val_gic_create_info_table
  @param   int_id Interrupt ID
  @return  none
**/
void val_gic_clear_interrupt(uint32_t int_id)
{
    uint32_t reg_offset = int_id / 32;
    uint32_t reg_shift  = int_id % 32;

    if (val_gic_is_valid_espi(int_id))
      val_bsa_gic_clear_espi_interrupt(int_id);
    else if ((int_id > 31) && (int_id < 1020)) {
        val_mmio_write(val_get_gicd_base() + GICD_ICPENDR0 + (4 * reg_offset), (1 << reg_shift));
        val_mmio_write(val_get_gicd_base() + GICD_ICACTIVER0 + (4 * reg_offset), (1 << reg_shift));
    }
    else
        val_print(ACS_PRINT_ERR, "\n    Invalid SPI interrupt ID number %d", int_id);
}

/**
  @brief   This function will initialize CPU interface registers required for interrupt
           routing to a given PE
           1. Caller       -  Test Suite
           2. Prerequisite -  val_gic_create_info_table
  @param   none
  @return  none
**/
void val_gic_cpuif_init(void)
{
  val_gic_reg_write(ICC_BPR1_EL1, 0x7);
  val_gic_reg_write(ICC_PMR_EL1, 0xff);
  val_gic_reg_write(ICC_IGRPEN1_EL1, 0x1);
}

/**
  @brief   This function will Get the trigger type Edge/Level
           1. Caller       -  Test Suite
           2. Prerequisite -  val_gic_create_info_table
  @param   int_id Interrupt ID
  @param   trigger_type to Store the Interrupt Trigger type
  @return  Status
**/
uint32_t val_gic_get_intr_trigger_type(uint32_t int_id, INTR_TRIGGER_INFO_TYPE_e *trigger_type)
{
  uint32_t reg_value;
  uint32_t reg_offset;
  uint32_t config_bit_shift;

  if (int_id > val_get_max_intid()) {
    val_print(ACS_PRINT_ERR, "\n       Invalid Interrupt ID number 0x%x ", int_id);
    return ACS_STATUS_ERR;
  }

  reg_offset = int_id / GICD_ICFGR_INTR_STRIDE;
  config_bit_shift  = GICD_ICFGR_INTR_CONFIG1(int_id);

  reg_value = val_mmio_read(val_get_gicd_base() + GICD_ICFGR + (4 * reg_offset));

  if ((reg_value & (1 << config_bit_shift)) == 0)
    *trigger_type = INTR_TRIGGER_INFO_LEVEL_HIGH;
  else
    *trigger_type = INTR_TRIGGER_INFO_EDGE_RISING;

  return 0;
}

/**
  @brief   This function will Get the trigger type Edge/Level for extended SPI int
           1. Caller       -  Test Suite
           2. Prerequisite -  val_gic_create_info_table
  @param   int_id Interrupt ID
  @param   trigger_type to Store the Interrupt Trigger type
  @return  Status
**/
uint32_t val_gic_get_espi_intr_trigger_type(uint32_t int_id,
                                                           INTR_TRIGGER_INFO_TYPE_e *trigger_type)
{
  uint32_t reg_value;
  uint32_t reg_offset;
  uint32_t config_bit_shift;

  if (!(int_id >= 4096 && int_id <= val_gic_max_espi_val())) {
    val_print(ACS_PRINT_ERR, "\n       Invalid Extended Int ID number 0x%x ", int_id);
    return ACS_STATUS_ERR;
  }

  /* 4096 is starting value of extended SPI int */
  reg_offset = (int_id - 4096) / GICD_ICFGR_INTR_STRIDE;
  config_bit_shift  = GICD_ICFGR_INTR_CONFIG1(int_id - 4096);

  reg_value = val_mmio_read(val_get_gicd_base() + GICD_ICFGRE + (4 * reg_offset));

  if ((reg_value & (1 << config_bit_shift)) == 0)
    *trigger_type = INTR_TRIGGER_INFO_LEVEL_HIGH;
  else
    *trigger_type = INTR_TRIGGER_INFO_EDGE_RISING;

  return 0;
}

/**
  @brief   This function will Set the trigger type Edge/Level based on the GTDT table
           1. Caller       -  Test Suite
           2. Prerequisite -  val_gic_create_info_table
  @param   int_id Interrupt ID
  @param   trigger_type Interrupt Trigger Type
  @return  none
**/
void val_gic_set_intr_trigger(uint32_t int_id, INTR_TRIGGER_INFO_TYPE_e trigger_type)
{
  uint32_t status;

  val_print(ACS_PRINT_DEBUG, "\n       Setting Trigger type as %d  ",
                                                                trigger_type);
  status = pal_gic_set_intr_trigger(int_id, trigger_type);

  if (status)
    val_print(ACS_PRINT_ERR, "\n       Error Could Not Configure Trigger Type",
                                                                            0);
}

/**
  @brief   This API returns if extended SPI supported in system
  @param   None
  @return  0 not supported, 1 supported
**/
uint32_t
val_gic_espi_supported(void)
{
  uint32_t espi_support;

  espi_support = val_bsa_gic_espi_support();

  val_print(ACS_PRINT_INFO, "\n    ESPI supported %d  ", espi_support);
  return espi_support;
}

/**
  @brief   This API returns max extended SPI interrupt value
  @param   None
  @return  max extended spi int value
**/
uint32_t
val_gic_max_espi_val(void)
{
  uint32_t max_espi_val;

  max_espi_val = val_bsa_gic_max_espi_val();

  val_print(ACS_PRINT_INFO, "\n    max ESPI value %d  ", max_espi_val);
  return max_espi_val;
}

/**
  @brief   This API returns max extended PPI interrupt value
  @param   None
  @return  max extended PPI int value
**/
uint32_t
val_gic_max_eppi_val(void)
{
  uint32_t max_eppi_val;

  max_eppi_val = val_bsa_gic_max_eppi_val();

  val_print(ACS_PRINT_INFO, "\n    max EPPI value %d  ", max_eppi_val);
  return max_eppi_val;
}

/**
  @brief  API used to check whether int_id is a espi interrupt
  @param  interrupt
  @return 1: espi interrupt, 0: non-espi interrupt
**/
uint32_t
val_gic_is_valid_espi(uint32_t int_id)
{
  return val_bsa_gic_check_espi_interrupt(int_id);
}

/**
  @brief  API used to check whether int_id is a Extended PPI
  @param  interrupt
  @return 1: eppi interrupt, 0: non-eppi interrupt
**/
uint32_t
val_gic_is_valid_eppi(uint32_t int_id)
{
  return val_bsa_gic_check_eppi_interrupt(int_id);
}
