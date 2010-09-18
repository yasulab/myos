################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir.mk
-include harib27f/winhelo3/subdir.mk
-include harib27f/winhelo2/subdir.mk
-include harib27f/winhelo/subdir.mk
-include harib27f/walk/subdir.mk
-include harib27f/type/subdir.mk
-include harib27f/tview/subdir.mk
-include harib27f/tek/subdir.mk
-include harib27f/stars2/subdir.mk
-include harib27f/stars/subdir.mk
-include harib27f/star1/subdir.mk
-include harib27f/sosu3/subdir.mk
-include harib27f/sosu2/subdir.mk
-include harib27f/sosu/subdir.mk
-include harib27f/notrec/subdir.mk
-include harib27f/noodle/subdir.mk
-include harib27f/mmlplay/subdir.mk
-include harib27f/lines/subdir.mk
-include harib27f/iroha/subdir.mk
-include harib27f/invader/subdir.mk
-include harib27f/hello4/subdir.mk
-include harib27f/hello3/subdir.mk
-include harib27f/haribote/subdir.mk
-include harib27f/gview/subdir.mk
-include harib27f/color2/subdir.mk
-include harib27f/color/subdir.mk
-include harib27f/chklang/subdir.mk
-include harib27f/calc/subdir.mk
-include harib27f/beepdown/subdir.mk
-include harib27f/bball/subdir.mk
-include harib27f/a/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: libmyOS.a

# Tool invocations
libmyOS.a: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC アーカイバー'
	ar -r "libmyOS.a" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(ARCHIVES) libmyOS.a
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
