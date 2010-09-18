################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../harib27f/iroha/iroha.c 

OBJS += \
./harib27f/iroha/iroha.o 

C_DEPS += \
./harib27f/iroha/iroha.d 


# Each subdirectory must supply rules for building sources it contributes
harib27f/iroha/%.o: ../harib27f/iroha/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C コンパイラー'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


