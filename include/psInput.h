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
	uint16_t	entprellzeit;		// Entprellzeit, bei Verwendung des Interrupts nicht verwendet
	uint16_t	switchLongTime;		// Zeit für Langer Tastendruck
	void (*cb)(void);		   		// Interrupt Service Routine als call back
	POLARITY	polarity;			// Polarität
	bool		irq;				// true --> Es wird ein Interrupt benutzt
};

class clIn {
	public:
		clIn();
		~clIn(){}

		void Init(stInput _Param);
		void runStatus(void);
		
		bool Status(void) {
			return bShort;
		}

		bool StatusLong(void) {
			return bLong;
		}

	private:
		void SetStatus(void);
		
		stInput Param;
		
		uint32_t u32Timer;
	 	
		uint16_t u16Status;
		uint16_t u16StatusOld;

		uint16_t u16InCount;
		static uint16_t u16InCountMax;
				
		bool bStatus;	// aktueller Status des Eingangs		
		bool bShort;
		bool bLong;
};

