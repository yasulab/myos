################################################################################
# 自動生成ファイルです。 編集しないでください!
################################################################################

# これらのツール呼び出しからの入力および出力をビルド変数へ追加します 
C_SRCS += \
../harib27f/memmap/memmap.c 

OBJ_SRCS += \
../harib27f/memmap/memmap.obj 

OBJS += \
./harib27f/memmap/memmap.o 

C_DEPS += \
./harib27f/memmap/memmap.d 


# サブディレクトリーはすべて、それ自身がコントリビュートするソースをビルドするためのルールを提供しなければなりません
harib27f/memmap/%.o: ../harib27f/memmap/%.c
	@echo 'ビルドするファイル: $<'
	@echo '呼び出し中: Cygwin C コンパイラー'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'ビルド完了: $<'
	@echo ' '


