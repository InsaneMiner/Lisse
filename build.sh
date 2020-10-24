BUILD_KERNEL_VERSION=0.0.1


nasm -f elf32 kernel.asm -o kernel_asm.o
gcc -fno-stack-protector -m32 -include 'stddef.h' -c kernel.c -o kernel_c.o
ld -m elf_i386 -T link.ld -o kernel kernel_asm.o kernel_c.o

rm -rf bin/iso
rm -rf bin




mkdir bin
mkdir bin/binary



mkdir bin/iso
mkdir bin/iso/system32
mkdir bin/iso/boot
mkdir bin/iso/boot/grub/
cp kernel bin/iso/system32/kernel-$BUILD_KERNEL_VERSION
cp grub.cfg bin/iso/boot/grub/grub.cfg
cp images/splash.png bin/iso/boot/grub/splash.png                                           




mv kernel bin/binary/kernel
mv kernel_c.o bin/binary/kernel_c.o
mv kernel_asm.o bin/binary/kernel_asm.o


grub-mkrescue -o Lisse.iso bin/iso