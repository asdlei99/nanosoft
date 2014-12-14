/*
* Модуль работы с таймерами/счетчиками/ШИМ ATmega16a
*
* Таймеры в AVR очень гибкие имеют множество настроек, цель данного модуля
* упростить решение типовых задач связанных с таймерами, счетчикам и ШИМ.
* 
* Данный модуль состоит только из заголовочного файла и не имеет собственного
* обработчика прерываний.
* 
* В ATmega16a имеем 3 счетчика:
*   Counter0 - 8 бит, один PWM-канал
*   Counter1 - 16 бит, два PWM-канала, захват счетчика
*   Counter2 - 8 бит, один PWM-канал, поддержка часового кварца
* 
* (c) Alex V. Zolotov <zolotov-alex@shamangrad.net>, 2014
*     Fill free to copy, to compile, to use, to redistribute etc on your own risk.
*/

#ifndef __IOM16A_TIMER_H_
#define __IOM16A_TIMER_H_

#define TIM0_CLOCK_DISABLE    0
#define TIM0_CLOCK_1          1
#define TIM0_CLOCK_8          2
#define TIM0_CLOCK_64         3
#define TIM0_CLOCK_256        4
#define TIM0_CLOCK_1024       5
#define TIM0_CLOCK_EXT_FALL   6
#define TIM0_CLOCK_EXT_RISE   7

#define TIM0_FAST_PWM_NOPE      0
#define TIM0_FAST_PWM_RESERVED  1
#define TIM0_FAST_PWM_NORMAL    2
#define TIM0_FAST_PWM_INVERT    3

#define TIM0_FC_PWM_NOPE      0
#define TIM0_FC_PWM_RESERVED  1
#define TIM0_FC_PWM_NORMAL    2
#define TIM0_FC_PWM_INVERT    3

typedef unsigned char tim0_t;
typedef unsigned int tim1_t;
typedef unsigned char tim2_t;

/**
* Прочитать значение счетчика Counter0
*/
inline tim0_t tim0_get_counter()
{
	return TCNT0;
}

/**
* Установить значение счетчика Counter0
*/
inline void tim0_set_counter(tim0_t value)
{
	TCNT0 = value;
}

/**
* Вернуть значение с которым сравнивается счетик Counter0
*/
inline tim0_t tim0_get_compare()
{
	return OCR0;
}

/**
* Установить значение сравнения счетчика Counter0
*/
inline void tim0_set_compare(tim0_t value)
{
	OCR0 = value;
}

/**
* Включить прерывание по совпадению считчика Counter0
*/
inline void tim0_enable_compare_interrupt()
{
	TIMSK |= (1 << OCIE0);
}

/**
* Выключить прерывание по совпадению счетчика Counter0
*/
inline void tim0_disable_compare_interrupt()
{
	TIMSK &= ~(1 << OCIE0);
}

/**
* Включить/выключить прерывание по совпадению счетчика Counter0
*/
inline void tim0_set_compare_interrupt(bool value)
{
	if ( value )
	{
		tim0_enable_compare_interrupt();
	}
	else
	{
		tim0_disable_compare_interrupt();
	}
}

/**
* Проверить включены ли прерывания по совпадению счетчика Counter0
*/
inline bool tim0_get_compare_interrupt()
{
	return TIMSK & (1 << OCIE0);
}

/**
* Включить прерывание по переполнению счетчика Counter0
*/
inline void tim0_enable_overflow_interrupt()
{
	TIMSK |= (1 << TOIE0);
}

/**
* Выключить прерывание по переполнению счетчика Counter0
*/
inline void tim0_disable_overflow_interrupt()
{
	TIMSK &= ~(1 << TOIE0);
}

/**
* Включить/выключить прерывание по переполнению счетчика Counter0
*/
inline void tim0_set_overflow_interrupt(bool value)
{
	if ( value )
	{
		tim0_enable_overflow_interrupt();
	}
	else
	{
		tim0_disable_overflow_interrupt();
	}
}

/**
* Инициализация простого счетчика
* @param clock прескейлер счетчика
*/
inline void tim0_normal(char clock)
{
	TCCR0 = (0 << WGM01) | (0 << WGM00) | (clock & 0x07);
}

/**
* Инициализация CTC-счетчика (Clear Timer on Compare)
* @param clock прескейлер счетчика
*/
inline void tim0_ctc(char clock)
{
	TCCR0 = (1 << WGM01) | (0 << WGM00) | (clock & 0x07);
}

/**
* Инициализация Fast PWM
* @param clock прескейлер счетчика
* @param mode режим генерации сигнала
*/
inline void tim0_fast_pwm(char clock, char mode = TIM0_FAST_PWM_NORMAL)
{
	TCCR0 = (1 << WGM01) | (1 << WGM00) | ((mode & 0x03) << COM00) | (clock & 0x07);
	DDRB |= (1 << PB3);
}

/**
* Инициализация Phase Correct PWM
* @param clock прескейлер счетчика
* @param mode режим генерации сигнала
*/
inline void tim0_fc_pwm(char clock, char mode = TIM0_FC_PWM_NORMAL)
{
	TCCR0 = (0 << WGM01) | (1 << WGM00) | ((mode & 0x03) << COM00) | (clock & 0x07);
	DDRB |= (1 << PB3);
}

/**
* Прочитать значение счетчика Counter1
*/
inline tim1_t tim1_get_counter()
{
	return TCNT1;
}

/**
* Установить значение счетчика Counter1
*/
inline void tim1_set_counter(tim1_t value)
{
	TCNT1 = value;
}

/**
* Вернуть A-значение с которым сравнивается счетик Counter1
*/
inline tim1_t tim1_get_compareA()
{
	return OCR1A;
}

/**
* Вернуть B-значение с которым сравнивается счетик Counter1
*/
inline tim1_t tim1_get_compareB()
{
	return OCR1B;
}

/**
* Установить A-значение сравнения счетчика Counter1
*/
inline void tim1_set_compareA(tim1_t value)
{
	OCR1A = value;
}

/**
* Установить B-значение сравнения счетчика Counter1
*/
inline void tim1_set_compareB(tim1_t value)
{
	OCR1B = value;
}

/**
* Вернуть значение захваченного значения счетчика Counter1
*/
inline tim1_t tim1_get_capture()
{
	return ICR1;
}

/**
* Включить прерывание по совпадению считчика Counter1 со значением A
*/
inline void tim1_enable_compareA_interrupt()
{
	TIMSK |= (1 << OCIE1A);
}

/**
* Включить прерывание по совпадению считчика Counter1 со значением B
*/
inline void tim1_enable_compareB_interrupt()
{
	TIMSK |= (1 << OCIE1B);
}

/**
* Выключить прерывание по совпадению счетчика Counter1 со значением A
*/
inline void tim1_disable_compareA_interrupt()
{
	TIMSK &= ~(1 << OCIE1A);
}

/**
* Выключить прерывание по совпадению счетчика Counter1 со значением B
*/
inline void tim1_disable_compareB_interrupt()
{
	TIMSK &= ~(1 << OCIE1B);
}

/**
* Включить прерывание по захвату счетчика Counter1
*/
inline void tim1_enable_capture_interrupt()
{
	TIMSK |= (1 << TICIE1);
}

/**
* Выключить прерывание по захвату счетчика Counter1
*/
inline void tim1_disable_capture_interrupt()
{
	TIMSK &= ~(1 << TICIE1);
}

/**
* Включить прерывание по переполнению счетчика Counter1
*/
inline void tim1_enable_overflow_interrupt()
{
	TIMSK |= (1 << TOIE1);
}

/**
* Выключить прерывание по переполнению счетчика Counter1
*/
inline void tim1_disable_overflow_interrupt()
{
	TIMSK &= ~(1 << TOIE1);
}

#endif // __IOM16A_TIMER_H_
