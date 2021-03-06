/**
 * Copyright (C) 2021 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file    no_motion.c
 * @brief   Test code to demonstrate on how to configure and use no-motion feature
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include "bmi090l.h"
#include "common.h"

/*********************************************************************/
/*                       Function Declarations                       */
/*********************************************************************/

/*!
 * @brief    This internal API is used to initialize the bmi090l sensor
 */
static void init_bmi090l(struct bmi090l_dev *bmi090ldev);

/*********************************************************************/
/*                          Functions                                */
/*********************************************************************/

/*!
 *  @brief This internal API is used to initializes the bmi090l sensor
 *
 *  @param[in] void
 *
 *  @return void
 *
 */
static void init_bmi090l(struct bmi090l_dev *bmi090ldev)
{
    int8_t rslt = BMI090L_OK;

    /* Initialize bmi090l sensors (accel & gyro) */
    if (bmi090la_init(bmi090ldev) == BMI090L_OK && bmi090lg_init(bmi090ldev) == BMI090L_OK)
    {
        printf("BMI090L initialization success!\n");
        printf("Accel chip ID - 0x%x\n", bmi090ldev->accel_chip_id);
        printf("Gyro chip ID - 0x%x\n", bmi090ldev->gyro_chip_id);

        /* Reset the accelerometer */
        rslt = bmi090la_soft_reset(bmi090ldev);
    }
    else
    {
        printf("BMI090L initialization failure!\n");
        exit(COINES_E_FAILURE);
    }

    /*! Max read/write length (maximum supported length is 32).
     * To be set by the user */
    bmi090ldev->read_write_len = 32;

    /* Set accel power mode */
    bmi090ldev->accel_cfg.power = BMI090L_ACCEL_PM_ACTIVE;
    rslt = bmi090la_set_power_mode(bmi090ldev);

    if (rslt == BMI090L_OK)
    {
        bmi090ldev->gyro_cfg.power = BMI090L_GYRO_PM_NORMAL;
        bmi090lg_set_power_mode(bmi090ldev);
    }

    printf("Uploading config file !\n");

    /* API uploads the bmi090l config file onto the device */
    rslt = bmi090la_apply_config_file(bmi090ldev);

    if (rslt == BMI090L_OK)
    {
        printf("Upload done !\n");

        if (rslt == BMI090L_OK)
        {
            bmi090ldev->accel_cfg.bw = BMI090L_ACCEL_BW_NORMAL;
            bmi090ldev->accel_cfg.odr = BMI090L_ACCEL_ODR_200_HZ;
            bmi090ldev->accel_cfg.range = BMI090L_ACCEL_RANGE_6G;
            bmi090la_set_meas_conf(bmi090ldev);
        }
    }
}

static void configure_bmi090l_no_motion_interrupt(struct bmi090l_dev *bmi090ldev)
{
    struct bmi090l_no_motion_cfg no_motion_cfg = {};
    struct bmi090l_accel_int_channel_cfg no_motion_int_cfg = { };

    bmi090la_get_no_motion_config(&no_motion_cfg, bmi090ldev);

    /* Configure no-motion settings */
    no_motion_cfg.threshold = 0xAA; /* (0.124g * 2^15)/24g = 0xAA */
    no_motion_cfg.duration = 5; /* 100ms/20 = 5 */
    no_motion_cfg.enable = 1;
    no_motion_cfg.select_x = 1;
    no_motion_cfg.select_y = 1;
    no_motion_cfg.select_z = 1;
    bmi090la_set_no_motion_config(&no_motion_cfg, bmi090ldev);

    /* Map high-g interrupt to INT1 */
    no_motion_int_cfg.int_channel = BMI090L_INT_CHANNEL_1;
    no_motion_int_cfg.int_type = BMI090L_NO_MOTION_INT;
    no_motion_int_cfg.int_pin_cfg.output_mode = BMI090L_INT_MODE_PUSH_PULL;
    no_motion_int_cfg.int_pin_cfg.lvl = BMI090L_INT_ACTIVE_HIGH;
    no_motion_int_cfg.int_pin_cfg.enable_int_pin = BMI090L_ENABLE;
    bmi090la_set_int_config(&no_motion_int_cfg, bmi090ldev);
}

/*!
 *  @brief Main Function where the execution getting started to test the code.
 *
 *  @param[in] argc
 *  @param[in] argv
 *
 *  @return status
 *
 */
int main(int argc, char *argv[])
{
    struct bmi090l_dev bmi090l;
    int8_t rslt;
    uint8_t status = 0;
    uint8_t interrupt_count = 0;

    /* Interface reference is given as a parameter
     *         For I2C : BMI090L_I2C_INTF
     *         For SPI : BMI090L_SPI_INTF
     */
    rslt = bmi090l_interface_init(&bmi090l, BMI090L_SPI_INTF);
    bmi090l_check_rslt("bmi090l_interface_init", rslt);

    /* Initialize the sensors */
    init_bmi090l(&bmi090l);

    configure_bmi090l_no_motion_interrupt(&bmi090l);

    printf("Do not move the board to detect no-motion\n");

    while (1)
    {
        rslt = bmi090la_get_feat_int_status(&status, &bmi090l);
        if (status & BMI090L_ACCEL_NO_MOT_INT)
        {
            printf("No-motion detected %d\n", interrupt_count);
            interrupt_count++;
            if (interrupt_count == 10)
            {
                printf("No-motion testing done. Exiting! \n");
                break;
            }
        }
    }

    bmi090l_coines_deinit();

    return rslt;
}
