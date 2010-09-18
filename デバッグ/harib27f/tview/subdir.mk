################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../harib27f/tview/tview.c 

OBJS += \
./harib27f/tview/tview.o 

C_DEPS += \
./harib27f/tview/tview.d 


# Each subdirectory must supply rules for building sources it contributes
harib27f/tview/%.o: ../harib27f/tview/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C コンパイラー'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


