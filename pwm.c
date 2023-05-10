/* ************************************************************************** */
/** PWM

  @Company
    ShimaneJohoshoriCenter.inc

  @File Name
    pwm.c

  @Summary
    PWM processing

  @Description
    mruby/c function army
 */
/* ************************************************************************** */

#include <limits.h>

#include "pic32mx.h"
#include "gpio.h"
#include "mrubyc.h"

#if !defined(UINT16_MAX)
#define UINT16_MAX 65535U
#endif


// DS60001168L  TABLE 11-2: OUTPUT PIN SELECTION
static const struct {
  unsigned int port : 4;
  unsigned int num : 4;
  unsigned int unit_num : 4;
} PWM_PIN_ASSIGN[] = {
		// OC1 group
  { 1, 0, 1 },	// RPA0
  { 2, 3, 1 },	// RPB3
  { 2, 4, 1 },	// RPB4
  { 2,15, 1 },	// RPB15
  { 2, 7, 1 },	// RPB7

		// OC2 group
  { 1, 1, 2 },	// RPA1
  { 2, 5, 2 },	// RPB5
  { 2, 1, 2 },	// RPB1
  { 2,11, 2 },	// RPB11
  { 2, 8, 2 },	// RPB8

		// OC4 group
  { 1, 2, 4 },	// RPA2
  { 2, 6, 4 },	// RPB6
  { 1, 4, 4 },	// RPA4
  { 2,13, 4 },	// RPB13
  { 2, 2, 4 },	// RPB2

		// OC3 group
  { 1, 3, 3 },	// RPA3
  { 2,14, 3 },	// RPB14
  { 2, 0, 3 },	// RPB0
  { 2,10, 3 },	// RPB10
  { 2, 9, 3 },	// RPB9
};
static const int NUM_PWM_PIN_ASSIGN = sizeof(PWM_PIN_ASSIGN) / sizeof(PWM_PIN_ASSIGN[0]);

/*
  PWM (OC) management data
 */
typedef struct PWM_HANDLE {
  PIN_HANDLE pin;
  uint8_t flag_in_use;
  uint8_t unit_num;	// 1..4
  uint16_t period;	// PRx set count value.
  uint16_t duty;	// percent but stretch 100% to UINT16_MAX
} PWM_HANDLE;

#define NUM_PWM_OC_UNIT 4
static PWM_HANDLE pwm_handle_[NUM_PWM_OC_UNIT];



/*! PWM set frequency
*/
static int pwm_set_frequency( PWM_HANDLE *pwm_h, double freq )
{
  if( freq == 0 ) {
    pwm_h->period = 0;
  } else {
    pwm_h->period = (PBCLK/4) / freq;
  }

  PR2 = pwm_h->period;
  OCxRS(pwm_h->unit_num) = (uint32_t)pwm_h->period * pwm_h->duty / UINT16_MAX;
  TMR2 = 0;

  return 0;
}

/*! PWM set period (us)
*/
static int pwm_set_period_us( PWM_HANDLE *pwm_h, unsigned int us )
{
  pwm_h->period = (uint64_t)us * (PBCLK/4) / 1000000;

  PR2 = pwm_h->period;
  OCxRS(pwm_h->unit_num) = (uint32_t)pwm_h->period * pwm_h->duty / UINT16_MAX;
  TMR2 = 0;

  return 0;
}


/*! PWM set duty cycle as percentage.
*/
static int pwm_set_duty( PWM_HANDLE *pwm_h, double duty )
{
  pwm_h->duty = duty / 100 * UINT16_MAX;

  OCxRS(pwm_h->unit_num) = pwm_h->period * duty / 100;

  return 0;
}


/*! PWM set pulse width.
*/
static int pwm_set_pulse_width_us( PWM_HANDLE *pwm_h, unsigned int us )
{
  OCxRS(pwm_h->unit_num) = (uint64_t)us * (PBCLK/4) / 1000000;

  return 0;
}


/* ============================= mruby/c codes ============================= */

/*! constructor

  pwm1 = PWM.new( num )		# num: pin number of Rboard
  pwm1 = PWM.new("A0")		# PIC origined pin assingment.
  pwm1 = PWM.new( 1, frequency:440, duty:30 )
*/
static void c_pwm_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  MRBC_KW_ARG(frequency, freq, duty);
  if( !MRBC_KW_END() ) goto RETURN;
  if( argc == 0 ) goto ERROR_RETURN;

  // allocate instance with PWM_HANDLE table pointer.
  mrbc_value val = mrbc_instance_new(vm, v[0].cls, sizeof(PWM_HANDLE *));
  SET_RETURN( val );

  PIN_HANDLE pin_h;
  if( set_pin_handle( &pin_h, &v[1] ) != 0 ) goto ERROR_RETURN;

  // find PWM(OC) unit from PWM_PIN_ASSIGN table.
  int i;
  for( i = 0; i < NUM_PWM_PIN_ASSIGN; i++ ) {
    if( (PWM_PIN_ASSIGN[i].port == pin_h.port) &&
	(PWM_PIN_ASSIGN[i].num == pin_h.num )) break;
  }
  if( i == NUM_PWM_PIN_ASSIGN ) goto ERROR_RETURN;

  int unit_num = PWM_PIN_ASSIGN[i].unit_num;

  // check already in use OC unit.
  if( pwm_handle_[unit_num-1].flag_in_use ) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "PWM already in use.");
    goto RETURN;
  }

  pwm_handle_[unit_num-1].pin = pin_h;
  pwm_handle_[unit_num-1].flag_in_use = 1;
  *(const PWM_HANDLE **)(val.instance->data) = &pwm_handle_[unit_num-1];

  // set pin to digital output
  gpio_setmode( &pin_h, GPIO_OUT );
  RPxnR( pin_h.port, pin_h.num ) = 0x05;  // 0x05: TABLE 11-2

  // set OC module
  OCxCON(unit_num) = 0x0006;	// PWM mode, use Timer2.
  OCxR(unit_num) = 0;
  OCxRS(unit_num) = 0;

  // set frequency and duty
  if( MRBC_ISNUMERIC(frequency) ) {
    pwm_set_frequency( &pwm_handle_[unit_num-1], MRBC_TO_FLOAT(frequency));
  }
  if( MRBC_ISNUMERIC(freq) ) {
    pwm_set_frequency( &pwm_handle_[unit_num-1], MRBC_TO_FLOAT(freq));
  }
  if( MRBC_ISNUMERIC(duty) ) {
    pwm_set_duty( &pwm_handle_[unit_num-1], MRBC_TO_FLOAT(duty));
  }

  OCxCON(unit_num) |= 0x8000;	// OC module ON
  goto RETURN;

 ERROR_RETURN:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), "PWM initialize.");

 RETURN:
  MRBC_KW_DELETE(frequency, freq, duty);
}


/*! set frequency

  pwm1.frequency( 440 )
*/
static void c_pwm_frequency(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PWM_HANDLE *h = *(PWM_HANDLE **)v[0].instance->data;

  if( MRBC_ISNUMERIC(v[1]) ) {
    pwm_set_frequency( h, MRBC_TO_FLOAT(v[1]));
  }
}


/*! set period by microsecond

  pwm1.period_us( 2273 )
*/
static void c_pwm_period_us(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PWM_HANDLE *h = *(PWM_HANDLE **)v[0].instance->data;

  if( MRBC_ISNUMERIC(v[1]) ) {
    pwm_set_period_us( h, MRBC_TO_INT(v[1]));
  }
}


/*! PWM set duty cycle as percentage.

  pwm1.duty( 50 )
*/
static void c_pwm_duty(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PWM_HANDLE *h = *(PWM_HANDLE **)v[0].instance->data;

  if( MRBC_ISNUMERIC(v[1]) ) {
    pwm_set_duty( h, MRBC_TO_FLOAT(v[1]));
  }
}


/*! PWM set pulse width by microsecond.

  pwm1.pulse_width_us( 20 )
*/
static void c_pwm_pulse_width_us(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PWM_HANDLE *h = *(PWM_HANDLE **)v[0].instance->data;

  if( MRBC_ISNUMERIC(v[1]) ) {
    pwm_set_pulse_width_us( h, MRBC_TO_INT(v[1]));
  }
}


/*! Initializer
*/
void mrbc_init_class_pwm(void)
{
  for( int i = 0; i < NUM_PWM_OC_UNIT; i++ ) {
    pwm_handle_[i].unit_num = i + 1;
    pwm_handle_[i].duty = UINT16_MAX / 2;
  }

  // using timer2, start.
  // TODO:
  //   現在 timer2 を OC1-4で共用していて周波数の設定が全て共通になっている。
  //   つまり Frequencyは1種類しか選べない。
  //   また、OCx は Timer2 か 3 しか選ぶことができないという制約がある。

  T2CON = 0x0020;	// 1:4 prescalor, 16bit
  TMR2 = 0;
  PR2 = 0xffff;
  T2CONSET = (1 << _T2CON_ON_POSITION);

  mrbc_class *pwm = mrbc_define_class(0, "PWM", 0);

  mrbc_define_method(0, pwm, "new", c_pwm_new);
  mrbc_define_method(0, pwm, "frequency", c_pwm_frequency);
  mrbc_define_method(0, pwm, "period_us", c_pwm_period_us);
  mrbc_define_method(0, pwm, "duty", c_pwm_duty);
  mrbc_define_method(0, pwm, "pulse_width_us", c_pwm_pulse_width_us);
}