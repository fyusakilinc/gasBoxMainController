################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/Hardware.c \
../Core/Src/func.c \
../Core/Src/func_binarypro.c \
../Core/Src/gasbox.c \
../Core/Src/gpio.c \
../Core/Src/iso.c \
../Core/Src/main.c \
../Core/Src/priority_pushpop.c \
../Core/Src/prioritylist.c \
../Core/Src/remote.c \
../Core/Src/resultqueue.c \
../Core/Src/spi.c \
../Core/Src/stacks.c \
../Core/Src/stm32g4xx_hal_msp.c \
../Core/Src/stm32g4xx_it.c \
../Core/Src/syscalls.c \
../Core/Src/sysmem.c \
../Core/Src/system_stm32g4xx.c \
../Core/Src/tim.c \
../Core/Src/timer0.c \
../Core/Src/uart4.c \
../Core/Src/usart.c \
../Core/Src/zentrale.c \
../Core/Src/zentrale_cmd_sero.c 

OBJS += \
./Core/Src/Hardware.o \
./Core/Src/func.o \
./Core/Src/func_binarypro.o \
./Core/Src/gasbox.o \
./Core/Src/gpio.o \
./Core/Src/iso.o \
./Core/Src/main.o \
./Core/Src/priority_pushpop.o \
./Core/Src/prioritylist.o \
./Core/Src/remote.o \
./Core/Src/resultqueue.o \
./Core/Src/spi.o \
./Core/Src/stacks.o \
./Core/Src/stm32g4xx_hal_msp.o \
./Core/Src/stm32g4xx_it.o \
./Core/Src/syscalls.o \
./Core/Src/sysmem.o \
./Core/Src/system_stm32g4xx.o \
./Core/Src/tim.o \
./Core/Src/timer0.o \
./Core/Src/uart4.o \
./Core/Src/usart.o \
./Core/Src/zentrale.o \
./Core/Src/zentrale_cmd_sero.o 

C_DEPS += \
./Core/Src/Hardware.d \
./Core/Src/func.d \
./Core/Src/func_binarypro.d \
./Core/Src/gasbox.d \
./Core/Src/gpio.d \
./Core/Src/iso.d \
./Core/Src/main.d \
./Core/Src/priority_pushpop.d \
./Core/Src/prioritylist.d \
./Core/Src/remote.d \
./Core/Src/resultqueue.d \
./Core/Src/spi.d \
./Core/Src/stacks.d \
./Core/Src/stm32g4xx_hal_msp.d \
./Core/Src/stm32g4xx_it.d \
./Core/Src/syscalls.d \
./Core/Src/sysmem.d \
./Core/Src/system_stm32g4xx.d \
./Core/Src/tim.d \
./Core/Src/timer0.d \
./Core/Src/uart4.d \
./Core/Src/usart.d \
./Core/Src/zentrale.d \
./Core/Src/zentrale_cmd_sero.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/%.o Core/Src/%.su Core/Src/%.cyclo: ../Core/Src/%.c Core/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32G473xx -c -I../Core/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc -I../Drivers/STM32G4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32G4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src

clean-Core-2f-Src:
	-$(RM) ./Core/Src/Hardware.cyclo ./Core/Src/Hardware.d ./Core/Src/Hardware.o ./Core/Src/Hardware.su ./Core/Src/func.cyclo ./Core/Src/func.d ./Core/Src/func.o ./Core/Src/func.su ./Core/Src/func_binarypro.cyclo ./Core/Src/func_binarypro.d ./Core/Src/func_binarypro.o ./Core/Src/func_binarypro.su ./Core/Src/gasbox.cyclo ./Core/Src/gasbox.d ./Core/Src/gasbox.o ./Core/Src/gasbox.su ./Core/Src/gpio.cyclo ./Core/Src/gpio.d ./Core/Src/gpio.o ./Core/Src/gpio.su ./Core/Src/iso.cyclo ./Core/Src/iso.d ./Core/Src/iso.o ./Core/Src/iso.su ./Core/Src/main.cyclo ./Core/Src/main.d ./Core/Src/main.o ./Core/Src/main.su ./Core/Src/priority_pushpop.cyclo ./Core/Src/priority_pushpop.d ./Core/Src/priority_pushpop.o ./Core/Src/priority_pushpop.su ./Core/Src/prioritylist.cyclo ./Core/Src/prioritylist.d ./Core/Src/prioritylist.o ./Core/Src/prioritylist.su ./Core/Src/remote.cyclo ./Core/Src/remote.d ./Core/Src/remote.o ./Core/Src/remote.su ./Core/Src/resultqueue.cyclo ./Core/Src/resultqueue.d ./Core/Src/resultqueue.o ./Core/Src/resultqueue.su ./Core/Src/spi.cyclo ./Core/Src/spi.d ./Core/Src/spi.o ./Core/Src/spi.su ./Core/Src/stacks.cyclo ./Core/Src/stacks.d ./Core/Src/stacks.o ./Core/Src/stacks.su ./Core/Src/stm32g4xx_hal_msp.cyclo ./Core/Src/stm32g4xx_hal_msp.d ./Core/Src/stm32g4xx_hal_msp.o ./Core/Src/stm32g4xx_hal_msp.su ./Core/Src/stm32g4xx_it.cyclo ./Core/Src/stm32g4xx_it.d ./Core/Src/stm32g4xx_it.o ./Core/Src/stm32g4xx_it.su ./Core/Src/syscalls.cyclo ./Core/Src/syscalls.d ./Core/Src/syscalls.o ./Core/Src/syscalls.su ./Core/Src/sysmem.cyclo ./Core/Src/sysmem.d ./Core/Src/sysmem.o ./Core/Src/sysmem.su ./Core/Src/system_stm32g4xx.cyclo ./Core/Src/system_stm32g4xx.d ./Core/Src/system_stm32g4xx.o ./Core/Src/system_stm32g4xx.su ./Core/Src/tim.cyclo ./Core/Src/tim.d ./Core/Src/tim.o ./Core/Src/tim.su ./Core/Src/timer0.cyclo ./Core/Src/timer0.d ./Core/Src/timer0.o ./Core/Src/timer0.su ./Core/Src/uart4.cyclo ./Core/Src/uart4.d ./Core/Src/uart4.o ./Core/Src/uart4.su ./Core/Src/usart.cyclo ./Core/Src/usart.d ./Core/Src/usart.o ./Core/Src/usart.su ./Core/Src/zentrale.cyclo ./Core/Src/zentrale.d ./Core/Src/zentrale.o ./Core/Src/zentrale.su ./Core/Src/zentrale_cmd_sero.cyclo ./Core/Src/zentrale_cmd_sero.d ./Core/Src/zentrale_cmd_sero.o ./Core/Src/zentrale_cmd_sero.su

.PHONY: clean-Core-2f-Src

