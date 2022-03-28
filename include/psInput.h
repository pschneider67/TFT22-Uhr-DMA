/*
 * psInput.h
 *
 *  Created on: 24.06.2020
 *      Author: pschneider
 */

#pragma once

struct stInput {
	uint16_t	pin;				// GPIO Nummer
	uint16_t	mode;				// Interrupt wird ausglöst bei Flanke oder Flankenwechsel
	uint16_t	entprellzeit;		// Entprellzeit, bei verwendung des Interrupts nicht verwendet
	void (*cb)(void);		   		// Interrupt Service Routine als call back
	POLARITY	polarity;			// Polarität
	bool		irq;				// true --> Es wird ein Interrupt benutzt
	bool      	status;				// aktueller Status des Eingangs
};

class clIn {
	public:
		clIn(){}
		~clIn(){}

		void Init(stInput *pParameter) {
			if (pParameter == NULL) {
				Serial.println("** --> input error");
				return;
			}
			pParam = pParameter;

			pinMode(pParam->pin, INPUT);

			if (pParam->irq) {
				noInterrupts();         		// Interrupts sperren
				if (pParam->polarity == POLARITY::POS) {
					attachInterrupt(digitalPinToInterrupt(pParam->pin), pParam->cb, pParam->mode);
				} else {
					attachInterrupt(digitalPinToInterrupt(pParam->pin), pParam->cb, pParam->mode);
				}
				Serial.println("** install irq for input " + (String)pParam->pin);
				interrupts();           		// Interrups freigeben
			}
		}

		bool Status(void) {
			if (pParam == NULL) {
				Serial.println("** --> input error");
			} else {
				if (pParam->irq) {			  	// Wenn Interrupt benutzt wird, dann ohne Entprellzeit
					SetStatus();				// Entprellung nur per Hardware
				} else {						// Mit Entprellzeit
					if (AktuelleZeit == 0) {	// 1. Durchlauf
						AktuelleZeit = millis();
						SetStatus();
					} else if ((millis() - AktuelleZeit) >= pParam->entprellzeit) {
						AktuelleZeit = 0;
						SetStatus();
					}
				}
			}
			return pParam->status;
		}

	private:
		void SetStatus(void) {
			if (pParam == NULL) {
				Serial.println("** --> input error");
				return;
			}

			if (pParam->polarity == POLARITY::POS) {
				pParam->status = digitalRead(pParam->pin);
			} else {
				pParam->status = ~digitalRead(pParam->pin);
			}
		}

		stInput *pParam = NULL;
		unsigned long AktuelleZeit = 0;
};
