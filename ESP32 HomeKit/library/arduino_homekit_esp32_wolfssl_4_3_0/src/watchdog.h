#ifndef WATCHDOG_H_
#define WATCHDOG_H_


#ifdef __cplusplus
extern "C" {
#endif

void watchdog_disable_all();

void watchdog_enable_all();

void watchdog_check_begin();

void watchdog_check_end(const char* message);

#ifdef __cplusplus
}
#endif


#endif /* WATCHDOG_H_ */
