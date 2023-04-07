/*
 * server.h
 *
 *  Created on: 24.03.2023
 *      Author: pschneider
 */

#pragma once

void handleIndex(void);
void handleValues(void);
void handleDelete(void); 
void handleConfig(void);
void handleWeather(void);
bool checkAuthentication(void);
void handleAuthentication(void);
bool getAuthentication(void);
void clearAuthentication(void);
void handleLogout(void);
