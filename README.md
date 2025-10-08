Continue from the SPI section of ad5592. Then examine the circuit and add what is missing from the code there ++
Implement the interlock ++ (hardware only ok)
Adjust the GPIOs of the pumps ++ (setstart and setstop)
Pump control block cmd ++, gasbox cmd++,


Implement the temperature sensor - I think this is done with the AD5592 pins, but I don't know how the algorithm will work ++
	- Yes, it's done with AD5592. The algorithm isn't too complicated based on the tmin and tmax you set on the device itself. Added ++
You can try adding RFG and match controllers ++? (this might be responsibility of Jianmin)
I don't know what the APC commands are = for PVC, these have been added but errors are likely, check thoroughly, you only activate one controller, do it based on input from the user
I can set and get the ISO pins, but what are they for? There are digital out in blocks for them ++ (They were for DIOs, added them to cmdlist)
Xport has not been added --
APC can be not init correctly. -- continue from the p commands, analyze get function and readline function, atm the get pos and set pos might not work because changed them to the p func, but not in cmd_sero
- when setting the apc, can try to read back to ensure it has correctly set it?