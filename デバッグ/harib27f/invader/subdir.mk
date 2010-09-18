################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../harib27f/invader/invader.c 

OBJS += \
./harib27f/invader/invader.o 

C_DEPS += \
./harib27f/invader/invader.d 


# Each subdirectory must supply rules for building sources it contributes
harib27f/invader/%.o: ../harib27f/invader/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C コンパイラー'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


