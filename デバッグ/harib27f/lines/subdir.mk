################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../harib27f/lines/lines.c 

OBJS += \
./harib27f/lines/lines.o 

C_DEPS += \
./harib27f/lines/lines.d 


# Each subdirectory must supply rules for building sources it contributes
harib27f/lines/%.o: ../harib27f/lines/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C �R���p�C���['
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


