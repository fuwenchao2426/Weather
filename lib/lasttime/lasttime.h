#include <Arduino.h>

#ifndef __FTask_H__
#define __FTask_H__

struct FTask {
	unsigned long lt;
	unsigned long hz = 1000;
	std::function<void(void)> func;

#define initHz(x, y) init(x, 1000.f / y)
#define initMs(x, y) init(x, y)
#define initS(x, y) init(x, y * 1000)
#define initM(x, y) init(x, y * 1000 * 60)
#define initH(x, y) init(x, y * 1000 * 60 * 60)

	void init(std::function<void(void)> f, unsigned long t) {
		func = f;
		func();
		hz = t;
	}

	/***********************
     * 
     *  lasttime aa;
     *  if(aa.check()){
     *      xxxxx;
     *  }
     * 
     * **********************/
	bool check() {
		if (millis() - lt >= hz) {
			lt = millis();
			return 1;
		}
		return 0;
	}

	/***********************
     * 
     *  lasttime aa;
     *  aa.initS(xxx,5);
     *  aa.run();
     * 
     * **********************/
	void run() {
		if (millis() - lt >= hz) {
			lt = millis();
			func();
		}
	}
};

#endif