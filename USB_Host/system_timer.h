/* INSERT LICENSE HERE */

#ifndef SYSTEM_TIMER_HPP_
#define SYSTEM_TIMER_HPP_

#include <stdint.h>
#include "Types.h"
#include "iodefine.h"

typedef enum{TIMER_ONESHOT= 0, TIMER_RELOAD = 1,} timer_type;
class System_Timer
{
private:
	static uint8_t numInstances;
	timer_type type;
	uint32_t reloadVal;	//Holds the value to reload into counter when underflow occurs
	uint32_t counter;	//Holds the current count
	void* arg;
	System_Timer *prev;
	void (*underflowCallBack)(void*);
protected:

public:

	System_Timer *next;
	System_Timer(uint32_t , void (*)(void*), void*, timer_type);
	~System_Timer();
	void tick(void);
	void halt(void);
	void resume(void);
	void restart(void);
};
#ifdef __cplusplus
extern "C" {
#endif

// DEFINITIONS ****************************************************************/

// DECLARATIONS ***************************************************************/

/**
 * Initialise the system timers.
 */
void sys_timer_start();
/**
 * Stop the timers.
 */
void sys_timer_stop();
/**
 * Service the millisecond timer.
 */
void sys_timer_service_ms();
/**
 * Delay execution for a number of ms.
 * @param   ms : number of ms to delay.
 */
void sys_timer_delay_ms(word ms);
/**
 * Set a ms timeout.
 * @param   duration : in ms.
 * @param   callback : when timeout served.
 */
void sys_timer_set_user_ms_timeout(word duration, callback_func_t callback);
/**
 * Getter for the elapsed milliseconds.
 * @return  milliseconds elapsed.
 */
word sys_timer_get_ms();
/**
 * Getter for the elapsed 100s of milliseconds.
 * @return  100s of milliseconds elapsed.
 */
word sys_timer_get_100ms();
/**
 * Getter for the elapsed microseconds.
 * @return  microseconds elapsed.
 */
word sys_timer_get_us();

void INT_Excep_CMT0_CMI0(void);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_TIMER_HPP_
