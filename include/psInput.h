/*
 * psInput.h
 *
 *  Created on: 24.06.2020
 *      Author: pschneider
 */

#pragma once

struct stInput {
	uint16_t	pin;				// number of GPIO 
	uint16_t	mode;				// irq at rising / falling edge 
	uint16_t	entprellzeit;		// debounce time, not active if irq is used
	uint16_t	switchLongTime;		// time to set a long push 
	void (*cb)(void);		   		// irq call back
	POLARITY	polarity;			// polarity of switch hardware
	bool		irq;				// true --> activate irq
};

class clIn {
	public:
		clIn();
		~clIn(){}

		void Init(stInput _Param);
		void runState(void);
		
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
				
		bool bStatus;	// actual state of switch		
		bool bShort;
		bool bLong;
};

