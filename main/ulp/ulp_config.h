/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
#pragma once

/* Ints are used here to be able to include the file in assembly as well */
#define ULP_ADC_CHANNEL         6  // ADC_CHANNEL_6, GPIO34 on ESP32, GPIO7 on ESP32-S3
#define ULP_ADC_UNIT            0  // ADC_UNIT_1
#define ULP_ADC_ATTEN           3  // ADC_ATTEN_DB_12
#define ULP_ADC_BITWIDTH        0  // ADC_BITWIDTH_DEFAULT
