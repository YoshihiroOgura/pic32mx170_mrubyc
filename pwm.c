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

/*! PWM pin assign table
*/
struct PWM_PIN_ASSIGN {
  uint8_t port;
  uint8_t num;
  uint8_t unit_num;	// 1..NUM_PWM_OC_UNIT
  uint8_t rpnr_value;
};

#if defined(__32MX170F256B__) || defined(__PIC32MX170F256B__)
// DS60001168L  TABLE 11-2: OUTPUT PIN SELECTION
static const struct PWM_PIN_ASSIGN PWM_PIN_ASSIGN[] =
{
  // OC1 group
  { 1, 0, 1, 0x5 },	// RPA0
  { 2, 3, 1, 0x5 },	// RPB3
  { 2, 4, 1, 0x5 },	// RPB4
  { 2,15, 1, 0x5 },	// RPB15
  { 2, 7, 1, 0x5 },	// RPB7

  // OC2 group
  { 1, 1, 2, 0x5 },	// RPA1
  { 2, 5, 2, 0x5 },	// RPB5
  { 2, 1, 2, 0x5 },	// RPB1
  { 2,11, 2, 0x5 },	// RPB11
  { 2, 8, 2, 0x5 },	// RPB8

  // OC4 group
  { 1, 2, 4, 0x5 },	// RPA2
  { 2, 6, 4, 0x5 },	// RPB6
  { 1, 4, 4, 0x5 },	// RPA4
  { 2,13, 4, 0x5 },	// RPB13
  { 2, 2, 4, 0x5 },	// RPB2

  // OC3 group
  { 1, 3, 3, 0x5 },	// RPA3
  { 2,14, 3, 0x5 },	// RPB14
  { 2, 0, 3, 0x5 },	// RPB0
  { 2,10, 3, 0x5 },	// RPB10
  { 2, 9, 3, 0x5 },	// RPB9
};
#else
#include "pwm_dependent.h"
#endif

/*
  PWM (OC) management data
 */
typedef struct PWM_HANDLE {
  PIN_HANDLE pin;
  uint8_t flag_in_use;
  uint8_t unit_num;
  uint16_t period;	// PRx set count value.
  uint16_t duty;	// percent but stretch 100% to UINT16_MAX
  uint8_t timer_num;	// Either 2 or 3 is specified due to hardware spec.
} PWM_HANDLE;

static PWM_HANDLE pwm_handle_[NUM_PWM_OC_UNIT];


/* set frequency, set period subroutine
*/
static int set_freq_sub( PWM_HANDLE *hndl )
{
  // set duty
  OCxRS(hndl->unit_num) = (uint32_t)hndl->period * hndl->duty / UINT16_MAX;

  // set frequency
  switch( hndl->timer_num ) {
  case 2:
    PR2 = hndl->period;
    if( hndl->period <= TMR2 ) TMR2 = 0;
    break;

  case 3:
    PR3 = hndl->period;
    if( hndl->period <= TMR3 ) TMR3 = 0;
    break;
  }

  return 0;
}


/*! PWM set frequency
*/
static int pwm_set_frequency( PWM_HANDLE *hndl, double freq )
{
  if( freq == 0 ) {
    hndl->period = 0;
  } else {
    hndl->period = (PBCLK/4) / freq;
  }

  return set_freq_sub( hndl );
}


/*! PWM set period (us)
*/
static int pwm_set_period_us( PWM_HANDLE *hndl, unsigned int us )
{
  hndl->period = (uint64_t)us * (PBCLK/4) / 1000000;

  return set_freq_sub( hndl );
}


/*! PWM set duty cycle as percentage.
*/
static int pwm_set_duty( PWM_HANDLE *hndl, double duty )
{
  hndl->duty = duty / 100 * UINT16_MAX;

  OCxRS(hndl->unit_num) = hndl->period * duty / 100;

  return 0;
}


/*! PWM set pulse width.
*/
static int pwm_set_pulse_width_us( PWM_HANDLE *hndl, unsigned int us )
{
  OCxRS(hndl->unit_num) = (uint64_t)us * (PBCLK/4) / 1000000;

  return 0;
}


/*! assign pin.
*/
static int pwm_assign_pin( const PIN_HANDLE *pin )
{
  // find PWM(OC) unit from PWM_PIN_ASSIGN table.
  static const int NUM = sizeof(PWM_PIN_ASSIGN) / sizeof(struct PWM_PIN_ASSIGN);
  int i;
  for( i = 0; i < NUM; i++ ) {
    if( (PWM_PIN_ASSIGN[i].port == pin->port) &&
	(PWM_PIN_ASSIGN[i].num == pin->num )) break;
  }
  if( i == NUM ) return -1;

  int unit_num = PWM_PIN_ASSIGN[i].unit_num;
  PWM_HANDLE *hndl = &pwm_handle_[unit_num-1];

  // check already in use OC unit.
  if( hndl->flag_in_use ) return -1;

  hndl->pin = *pin;
  hndl->flag_in_use = 1;

  // set pin to digital output
  gpio_setmode( pin, GPIO_OUT );
  RPxnR( pin->port, pin->num ) = PWM_PIN_ASSIGN[i].rpnr_value;

  return unit_num;
}


/* ============================= mruby/c codes ============================= */

/*! constructor

  pwm1 = PWM.new( num )		# num: pin number of Rboard
  pwm1 = PWM.new("A0")		# PIC origined pin assingment.
  pwm1 = PWM.new( 1, frequency:440, duty:30 )
*/
static void c_pwm_new(mrbc_vm *vm, mrbc_value v[], int argc)
{
  mrbc_value *arg_pin = MRBC_ARG(1);
  MRBC_KW_ARG(frequency, freq, duty, timer);
  MRBC_KW_END();

  double arg_freq = 0;
  double arg_duty = 0;
  int arg_timer = 2;
  if( MRBC_KW_ISVALID(frequency) ) arg_freq = MRBC_VAL_F(&frequency);
  if( MRBC_KW_ISVALID(freq) ) arg_freq = MRBC_VAL_F(&freq);
  if( MRBC_KW_ISVALID(duty) ) arg_duty = MRBC_VAL_F(&duty);
  if( MRBC_KW_ISVALID(timer) ) arg_timer = MRBC_VAL_I(&timer);
  if( mrbc_israised(vm) ) goto RETURN;

  PIN_HANDLE pin;
  if( set_pin_handle( &pin, arg_pin ) != 0 ) goto ERROR_RETURN;

  int unit_num = pwm_assign_pin( &pin );
  if( unit_num < 0 ) goto ERROR_RETURN;
  PWM_HANDLE *hndl = &pwm_handle_[unit_num-1];

  // allocate instance with PWM_HANDLE table pointer.
  v[0] = mrbc_instance_new(vm, v[0].cls, sizeof(PWM_HANDLE *));
  *MRBC_INSTANCE_DATA_PTR(v, PWM_HANDLE *) = hndl;

  if( !(arg_timer == 2 || arg_timer == 3) ) {
    mrbc_raise(vm, MRBC_CLASS(ArgumentError), "timer must be 2 or 3");
    goto RETURN;
  }
  hndl->timer_num = arg_timer;

  // set OC module
  unsigned int octsel = (arg_timer - 2) << _OC1CON_OCTSEL_POSITION;
  OCxCON(unit_num) = 0x0006 | octsel;	// PWM mode
  OCxR(unit_num) = 0;
  OCxRS(unit_num) = 0;

  // set frequency and duty.
  if( arg_freq != 0 ) pwm_set_frequency( hndl, arg_freq );
  if( arg_duty != 0 ) pwm_set_duty( hndl, arg_duty );

  OCxCON(unit_num) |= 0x8000;	// OC module ON
  goto RETURN;


 ERROR_RETURN:
  mrbc_raise(vm, MRBC_CLASS(ArgumentError), 0);

 RETURN:
  MRBC_KW_DELETE(frequency, freq, duty, timer);
}


/*! set frequency

  pwm1.frequency( 440 )
*/
static void c_pwm_frequency(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PWM_HANDLE *hndl = *MRBC_INSTANCE_DATA_PTR(v, PWM_HANDLE *);

  double freq = MRBC_ARG_F(1);
  if( mrbc_israised(vm) ) return;

  pwm_set_frequency( hndl, freq );
}


/*! set period by microsecond

  pwm1.period_us( 2273 )
*/
static void c_pwm_period_us(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PWM_HANDLE *hndl = *MRBC_INSTANCE_DATA_PTR(v, PWM_HANDLE *);

  unsigned int period = MRBC_ARG_I(1);
  if( mrbc_israised(vm) ) return;

  pwm_set_period_us( hndl, period );
}


/*! PWM set duty cycle as percentage.

  pwm1.duty( 50 )
*/
static void c_pwm_duty(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PWM_HANDLE *hndl = *MRBC_INSTANCE_DATA_PTR(v, PWM_HANDLE *);

  double duty = MRBC_ARG_F(1);
  if( mrbc_israised(vm) ) return;

  pwm_set_duty( hndl, duty );
}


/*! PWM set pulse width by microsecond.

  pwm1.pulse_width_us( 20 )
*/
static void c_pwm_pulse_width_us(mrbc_vm *vm, mrbc_value v[], int argc)
{
  PWM_HANDLE *hndl = *MRBC_INSTANCE_DATA_PTR(v, PWM_HANDLE *);

  unsigned int pw = MRBC_ARG_I(1);
  if( mrbc_israised(vm) ) return;

  pwm_set_pulse_width_us( hndl, pw );
}


/*! Initializer
*/
void mrbc_init_class_pwm(void)
{
  static const struct MRBC_DEFINE_METHOD_LIST method_list[] = {
    { "new", c_pwm_new },
    { "frequency", c_pwm_frequency },
    { "period_us", c_pwm_period_us },
    { "duty", c_pwm_duty },
    { "pulse_width_us", c_pwm_pulse_width_us },
  };

  for( int i = 0; i < NUM_PWM_OC_UNIT; i++ ) {
    pwm_handle_[i].unit_num = i + 1;
    pwm_handle_[i].duty = UINT16_MAX / 2;
  }

  // using timer2,3 start.
  // Only two types of cycles[timer] available.
  T2CON = T3CON = 0x0020;	// 1:4 prescalor, 16bit
  TMR2 = TMR3 = 0;
  PR2 = PR3 = 0xffff;
  T2CONSET = T3CONSET = (1 << _T2CON_ON_POSITION);

  mrbc_class *pwm = mrbc_define_class(0, "PWM", 0);
  mrbc_define_method_list(0, pwm, method_list, sizeof(method_list) / sizeof(method_list[0]));
}
