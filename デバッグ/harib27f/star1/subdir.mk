################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../harib27f/star1/star1.c 

OBJ_SRCS += \
../harib27f/star1/star1.obj 

OBJS += \
./harib27f/star1/star1.o 

C_DEPS += \
./harib27f/star1/star1.d 


# Each subdirectory must supply rules for building sources it contributes
harib27f/star1/%.o: ../harib27f/star1/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C コンパイラー'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


