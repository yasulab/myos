################################################################################
# ���������t�@�C���ł��B �ҏW���Ȃ��ł�������!
################################################################################

# �����̃c�[���Ăяo������̓��͂���яo�͂��r���h�ϐ��֒ǉ����܂� 
C_SRCS += \
../harib27f/memmap/memmap.c 

OBJ_SRCS += \
../harib27f/memmap/memmap.obj 

OBJS += \
./harib27f/memmap/memmap.o 

C_DEPS += \
./harib27f/memmap/memmap.d 


# �T�u�f�B���N�g���[�͂��ׂāA���ꎩ�g���R���g���r���[�g����\�[�X���r���h���邽�߂̃��[����񋟂��Ȃ���΂Ȃ�܂���
harib27f/memmap/%.o: ../harib27f/memmap/%.c
	@echo '�r���h����t�@�C��: $<'
	@echo '�Ăяo����: Cygwin C �R���p�C���['
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo '�r���h����: $<'
	@echo ' '


