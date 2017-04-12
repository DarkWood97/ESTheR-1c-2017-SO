################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Memoria.c \
../src/funcionesGenericas.c \
../src/funcionesSockets.c \
../src/socket.c 

OBJS += \
./src/Memoria.o \
./src/funcionesGenericas.o \
./src/funcionesSockets.o \
./src/socket.o 

C_DEPS += \
./src/Memoria.d \
./src/funcionesGenericas.d \
./src/funcionesSockets.d \
./src/socket.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2017-1c-caia/FuncionesGenericas" -I"/home/utnso/workspace/tp-2017-1c-caia/Socket" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


