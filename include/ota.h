/*
 * ota.h
 *
 *  Created on: 24.03.2023
 *      Author: pschneider
 */

#pragma once

void initOTA(void);
void cbOtaOnStart(void);
void cbOtaOnEnd(void);
void cbOtaOnProgress(unsigned int progress, unsigned int total);
void cbOtaOnError(ota_error_t error);
