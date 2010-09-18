/* �������֌W */

#include "bootpack.h"
#include <stdio.h>
#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000

/**
 * return area of available memory
 */
unsigned int memtest(unsigned int start, unsigned int end)
{
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	/* 386���A486�ȍ~�Ȃ̂��̊m�F */
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT; /* AC-bit = 1 */
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if ((eflg & EFLAGS_AC_BIT) != 0) { /* 386�ł�AC=1�ɂ��Ă�������0�ɖ߂��Ă��܂� */
		flg486 = 1;
	}
	eflg &= ~EFLAGS_AC_BIT; /* AC-bit = 0 */
	io_store_eflags(eflg);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE; /* �L���b�V���֎~ */
		store_cr0(cr0);
	}

	i = memtest_sub(start, end);

	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE; /* �L���b�V������ */
		store_cr0(cr0);
	}

	return i;
}

void memman_init(struct MEMMAN *man)
{
	man->frees = 0;			/* �������̌� */
	man->maxfrees = 0;		/* �󋵊ώ@�p�Ffrees�̍ő�l */
	man->lostsize = 0;		/* ����Ɏ��s�������v�T�C�Y */
	man->losts = 0;			/* ����Ɏ��s������ */
	return;
}

/* �����T�C�Y�̍��v��� */
unsigned int memman_total(struct MEMMAN *man)
{
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}

/* ���������m�ۂ���֐�
 * @param man: �������}�l�[�W��
 * @param size: �f�[�^�T�C�Y
 * return �m�ۂ����������̈�̐擪�Ԓn */
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
{
	unsigned int i, a;
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].size >= size) {
			/* �\���ȍL���̂����𔭌� */
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0) {
				/* free[i]���Ȃ��Ȃ����̂őO�ւ߂� */
				man->frees--;
				/* �������i�ȍ~�̒l�ɑ΂���for�����s�� */
				for (; i < man->frees; i++) {
					man->free[i] = man->free[i + 1]; /* �\���̂̑�� */
				}
			}
			return a;
		}
	}
	return 0; /* �������Ȃ� */
}

/* ����������֐�
 * return 0: �����I��
 * return -1: ���s�I��*/
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i, j;
	/* �܂Ƃ߂₷�����l����ƁAfree[]��addr���ɕ���ł���ق������� */
	/* ������܂��A�ǂ��ɓ����ׂ��������߂� */
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}
	/* free[i - 1].addr < addr < free[i].addr */
	if (i > 0) {
		/* �O������ */
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			/* �O�̂����̈�ɂ܂Ƃ߂��� */
			man->free[i - 1].size += size;
			if (i < man->frees) {
				/* �������� */
				if (addr + size == man->free[i].addr) {
					/* �Ȃ�ƌ��Ƃ��܂Ƃ߂��� */
					man->free[i - 1].size += man->free[i].size;
					/* man->free[i]�̍폜 */
					/* free[i]���Ȃ��Ȃ����̂őO�ւ߂� */
					man->frees--;
					for (; i < man->frees; i++) {
						man->free[i] = man->free[i + 1]; /* �\���̂̑�� */
					}
				}
			}
			return 0; /* �����I�� */
		}
	}
	/* �O�Ƃ͂܂Ƃ߂��Ȃ����� */
	if (i < man->frees) {
		/* ��낪���� */
		if (addr + size == man->free[i].addr) {
			/* ���Ƃ͂܂Ƃ߂��� */
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; /* �����I�� */
		}
	}
	/* �O�ɂ����ɂ��܂Ƃ߂��Ȃ� */
	if (man->frees < MEMMAN_FREES) {
		/* free[i]�������A���ւ��炵�āA�����܂���� */
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees; /* �ő�l���X�V */
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0; /* �����I�� */
	}
	/* ���ɂ��点�Ȃ����� */
	man->losts++;
	man->lostsize += size;
	return -1; /* ���s�I�� */
}

/* ���������m�ۂ���֐�(wrapper)
 * @param man: �������}�l�[�W��
 * @param size: �m�ۂ���f�[�^�T�C�Y[4,096 byte]
 * return �m�ۂ����������̈�̔Ԓn�A�h���X
 * */
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
	unsigned int a;
	size = (size + 0xfff) & 0xfffff000;	// �[���J��グ
	a = memman_alloc(man, size);
	return a;
}

/* ����������֐�
 * return 0: �����I��
 * return -1: ���s�I�� */
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i;
	size = (size + 0xfff) & 0xfffff000;
	i = memman_free(man, addr, size);
	return i;
}


/* �������̎g�p�󋵂𕶎���ŕԂ� */
unsigned int memman_show(struct CONSOLE *cons, struct MEMMAN *man){
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}

/* �^����ꂽ�t�@�C���f�[�^�����̃t�@�C���f�[�^���������������
 * @param man: memory manager
 * @param middle_fdata: ������&������������������ԃt�@�C���f�[�^
 * return 1 if it succeeded.
 * return 0 if it failed.
 * */
int memman_free_fdata(struct MEMMAN *memman, unsigned int fdata_addr){
	struct MYFILEDATA *temp_fdata, *prev_temp_fdata;
	int i;
	char s[50];
	unsigned int fdata_size;

	fdata_size = sizeof(struct MYFILEDATA);
	temp_fdata = (struct MYFILEDATA *)fdata_addr;

	do{ /* ���̃t�@�C���f�[�^�����݂��Ă���� �A����������������s��*/
		prev_temp_fdata = temp_fdata;
		i = memman_free(memman, (unsigned int)temp_fdata, fdata_size);
		if(i == -1){
			sprintf(s, "return memman_free() was failed.\n");
			debug_print(s);
			return 0;
		}else{
			sprintf(s, "%d length was released from the addr 0x%08x\n", fdata_size, temp_fdata);
			debug_print(s);
		}
		temp_fdata = temp_fdata->head.next_fdata;
	}while(prev_temp_fdata->head.next_fdata != 0);

	return 1;
}
