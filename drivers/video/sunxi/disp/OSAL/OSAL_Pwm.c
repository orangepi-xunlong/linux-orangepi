#include "OSAL_Pwm.h"


#ifndef __OSAL_PWM_MASK__
__hdle OSAL_Pwm_request(u32 pwm_id)
{
	struct pwm_device *pwm_dev;

	pwm_dev = pwm_request(pwm_id, "lcd");

	if(NULL == pwm_dev || IS_ERR(pwm_dev)) {
		__wrn("OSAL_Pwm_request pwm %d fail!\n", pwm_id);
	} else {
		__inf("OSAL_Pwm_request pwm %d success!\n", pwm_id);
	}

	return (__hdle)pwm_dev;
}

int OSAL_Pwm_free(__hdle p_handler)
{
	int ret = 0;
	struct pwm_device *pwm_dev;

	pwm_dev = (struct pwm_device *)p_handler;
	if(NULL == pwm_dev || IS_ERR(pwm_dev)) {
		__wrn("OSAL_Pwm_free, handle is NULL!\n");
		ret = -1;
	} else {
		pwm_free(pwm_dev);
		__inf("OSAL_Pwm_free pwm %d \n", pwm_dev->pwm);
	}

	return ret;
}

int OSAL_Pwm_Enable(__hdle p_handler)
{
	int ret = 0;
	struct pwm_device *pwm_dev;

	pwm_dev = (struct pwm_device *)p_handler;
	if(NULL == pwm_dev || IS_ERR(pwm_dev)) {
		__wrn("OSAL_Pwm_Enable, handle is NULL!\n");
		ret = -1;
	} else {
		ret = pwm_enable(pwm_dev);
		__inf("OSAL_Pwm_Enable pwm %d \n", pwm_dev->pwm);
	}

	return ret;
}

int OSAL_Pwm_Disable(__hdle p_handler)
{
	int ret = 0;
	struct pwm_device *pwm_dev;

	pwm_dev = (struct pwm_device *)p_handler;
	if(NULL == pwm_dev || IS_ERR(pwm_dev)) {
		__wrn("OSAL_Pwm_Disable, handle is NULL!\n");
		ret = -1;
	} else {
		pwm_disable(pwm_dev);
		__inf("OSAL_Pwm_Disable pwm %d \n", pwm_dev->pwm);
	}

	return ret;
}

int OSAL_Pwm_Config(__hdle p_handler, int duty_ns, int period_ns)
{
	int ret = 0;
	struct pwm_device *pwm_dev;

	pwm_dev = (struct pwm_device *)p_handler;
	if(NULL == pwm_dev || IS_ERR(pwm_dev)) {
		__wrn("OSAL_Pwm_Config, handle is NULL!\n");
		ret = -1;
	} else {
		ret = pwm_config(pwm_dev, duty_ns, period_ns);
		__debug("OSAL_Pwm_Config pwm %d, <%d / %d> \n", pwm_dev->pwm, duty_ns, period_ns);
	}

	return ret;
}

int OSAL_Pwm_Set_Polarity(__hdle p_handler, int polarity)
{
	int ret = 0;
	struct pwm_device *pwm_dev;

	pwm_dev = (struct pwm_device *)p_handler;
	if(NULL == pwm_dev || IS_ERR(pwm_dev)) {
		__wrn("OSAL_Pwm_Set_Polarity, handle is NULL!\n");
		ret = -1;
	} else {
		ret = pwm_set_polarity(pwm_dev, polarity);
		__inf("OSAL_Pwm_Set_Polarity pwm %d, active %s\n", pwm_dev->pwm, (polarity==0)? "high":"low");
	}

	return ret;
}

#else
__hdle OSAL_Pwm_request(u32 pwm_id)
{
	int ret = 0;

	return ret;
}

int OSAL_Pwm_free(__hdle p_handler)
{
	int ret = 0;

	return ret;
}

int OSAL_Pwm_Enable(__hdle p_handler)
{
	int ret = 0;

	return ret;
}

int OSAL_Pwm_Disable(__hdle p_handler)
{
	int ret = 0;

	return ret;
}

int OSAL_Pwm_Config(__hdle p_handler, int duty_ns, int period_ns)
{
	int ret = 0;

	return ret;
}

int OSAL_Pwm_Set_Polarity(__hdle p_handler, int polarity)
{
	int ret = 0;

	return ret;
}

#endif
