/*
 * psOutput.h
 *
 *  Created on: 24.06.2020
 *      Author: pschneider
 */

#pragma once

class clOut {
	public:
		clOut(){}
		~clOut(){}

		void Init(uint16_t pin, POLARITY polarity) {
			OutPin = pin;
			Polarity = polarity;
			pinMode(OutPin, OUTPUT);
			Off();
		}

		void SwPort(bool Status) {
			if (Status) {
				On();
			} else {
				Off();
			}
		}

		void On(void) {
			if (Polarity == POLARITY::POS) {
				digitalWrite(OutPin, HIGH);
			} else {
				digitalWrite(OutPin, LOW);
			}
			Status = true;
		}

		void Off(void) {
			if (Polarity == POLARITY::POS) {
				digitalWrite(OutPin, LOW);
			} else {
				digitalWrite(OutPin, HIGH);
			}
			Status = false;
		}

		void Toggle(void) {
			if (Status) {
				Off();
				Status = false;
			} else {
				On();
				Status = true;
			}
		}	

		void Flash(bool Reset, uint16_t flashTime) {
			if (!Reset) {
				Off();
				FlashStatus = 0;
				return;
			}

			switch (FlashStatus) {
				case 0:
					AktuelleZeit = millis();
					On();
					FlashStatus = 10;
					break;
				case 10:
					if ((millis() - AktuelleZeit) >= flashTime) {
						Off();
						FlashStatus = 20;
					}
					break;
				case 20:			// wait for Reset false
					break;
				default:
					FlashStatus = 0;
					break;
			}
		}
	
	private:
		bool     Status = false;
		unsigned long AktuelleZeit = 0;
		uint16_t OutPin = 0;
		uint16_t FlashStatus = 0;
		POLARITY Polarity = POLARITY::POS;
};

