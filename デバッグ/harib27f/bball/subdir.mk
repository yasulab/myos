################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../harib27f/bball/bball.c 

OBJS += \
./harib27f/bball/bball.o 

C_DEPS += \
./harib27f/bball/bball.d 


# Each subdirectory must supply rules for building sources it contributes
harib27f/bball/%.o: ../harib27f/bball/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C コンパイラー'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


