################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../harib27f/notrec/notrec.c 

OBJS += \
./harib27f/notrec/notrec.o 

C_DEPS += \
./harib27f/notrec/notrec.d 


# Each subdirectory must supply rules for building sources it contributes
harib27f/notrec/%.o: ../harib27f/notrec/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C コンパイラー'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


