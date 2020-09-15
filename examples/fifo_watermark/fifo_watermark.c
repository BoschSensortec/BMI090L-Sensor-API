/**
 * Copyright (C) 2020 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file    bmi090l_fifo_watermark.c
 * @brief   Test code to demonstrate on how to configure and use fifo watermark feature
 *
 */

#include <stdio.h>
#include "coines.h"
#include <stdlib.h>
#include "bmi090l.h"
#include "common.h"

/*********************************************************************/
/* function declarations */
/*********************************************************************/

/*!
 * @brief    This internal API is used to initialize the bmi090l sensor
 */
static void init_bmi090l(struct bmi090l_dev *bmi090ldev);

/*********************************************************************/
/* functions */
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
    rslt = bmi090la_apply_config_file(bmi090ldev);

    /* API uploads the bmi090l config file onto the device */
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

static void configure_bmi090l_fifo_wm_interrupt(struct bmi090l_dev *bmi090ldev)
{
    int8_t rslt;
    uint16_t wml = 0;
    struct bmi090l_accel_fifo_config config;
    struct bmi090l_accel_int_channel_cfg int_config;

    /* Configure the Interrupt configurations */
    int_config.int_channel = BMI090L_INT_CHANNEL_1;
    int_config.int_type = BMI090L_FIFO_WM_INT;
    int_config.int_pin_cfg.lvl = BMI090L_INT_ACTIVE_HIGH;
    int_config.int_pin_cfg.output_mode = BMI090L_INT_MODE_PUSH_PULL;
    int_config.int_pin_cfg.enable_int_pin = BMI090L_ENABLE;

    /* Set the interrupt configuration */
    rslt = bmi090la_set_int_config(&int_config, bmi090ldev);
    bmi090l_check_rslt("bmi090la_set_int_config", rslt);

    /* Set water mark(aka 6 frames, each 7 bytes: 1 byte header + 6 bytes accel data) */
    wml = 1000;
    rslt = bmi090la_set_fifo_wm(wml, bmi090ldev);
    bmi090l_check_rslt("bmi090la_set_fifo_wm", rslt);

    printf("Watermark_data: %d\n", wml);

    config.accel_en = BMI090L_ENABLE;
    config.int1_en = BMI090L_ENABLE;

    /* Set FIFO configuration by enabling accelerometer */
    rslt = bmi090la_set_fifo_config(&config, bmi090ldev);
    bmi090l_check_rslt("bmi090la_set_fifo_config", rslt);
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
    uint8_t try = 1;
    uint16_t idx = 0;
    uint16_t accel_length = 100;
    uint16_t fifo_length = 0;

    uint8_t fifo_data[1024] = { 0 };
    struct bmi090l_fifo_frame fifo_frame = { 0 };
    struct bmi090l_sensor_data bmi090l_accel[100] = { { 0 } };

    /* Interface reference is given as a parameter
     *         For I2C : BMI090L_I2C_INTF
     *         For SPI : BMI090L_SPI_INTF
     */
    rslt = bmi090l_interface_init(&bmi090l, BMI090L_I2C_INTF);
    bmi090l_check_rslt("bmi090l_interface_init", rslt);

    /* Initialize the sensors */
    init_bmi090l(&bmi090l);

    configure_bmi090l_fifo_wm_interrupt(&bmi090l);

    /* Update FIFO structure */
    fifo_frame.data = fifo_data;
    fifo_frame.length = 1024;

    while (try <= 10)
    {
        bmi090l.delay_us(1000000, bmi090l.intf_ptr);

        rslt = bmi090la_get_data_int_status(&status, &bmi090l);
        bmi090l_check_rslt("bmi090la_get_data_int_status", rslt);

        if (status & BMI090L_ACCEL_FIFO_WM_INT)
        {
            printf("Iteration : %d\n", try);

            /* Read FIFO data */
            rslt = bmi090la_read_fifo_data(&fifo_frame, &bmi090l);
            bmi090l_check_rslt("bmi090la_read_fifo_data", rslt);

            if (rslt == BMI090L_OK)
            {
                rslt = bmi090la_get_fifo_length(&fifo_length, &bmi090l);
                bmi090l_check_rslt("bmi090la_get_fifo_length", rslt);

                printf("Requested data frames before parsing: %d\n", accel_length);
                printf("FIFO length available : %d\n", fifo_length);

                /* Parse the FIFO data to extract accelerometer data from the FIFO buffer */
                rslt = bmi090la_extract_accel(bmi090l_accel, &accel_length, &fifo_frame, &bmi090l);
                bmi090l_check_rslt("bmi090la_extract_accel", rslt);

                printf("Parsed accelerometer frames: %d\n", accel_length);

                /* Print the parsed accelerometer data from the FIFO buffer */
                for (idx = 0; idx < accel_length; idx++)
                {
                    printf("ACCEL[%d] X : %d\t Y : %d\t Z : %d\n",
                           idx,
                           bmi090l_accel[idx].x,
                           bmi090l_accel[idx].y,
                           bmi090l_accel[idx].z);
                }
            }
        }

        try++;
    }

    bmi090l_coines_deinit();

    return rslt;
}
