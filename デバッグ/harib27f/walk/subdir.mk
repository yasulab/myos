################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../harib27f/walk/walk.c 

OBJS += \
./harib27f/walk/walk.o 

C_DEPS += \
./harib27f/walk/walk.d 


# Each subdirectory must supply rules for building sources it contributes
harib27f/walk/%.o: ../harib27f/walk/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C コンパイラー'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


