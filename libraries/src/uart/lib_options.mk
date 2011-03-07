# Compile uart0.rel and uart1.rel from uart.c
LIB_RELS := libraries/src/uart/uart0.rel libraries/src/uart/uart1.rel

libraries/src/uart/uart0.rel : C_FLAGS += -DUART0
libraries/src/uart/uart1.rel : C_FLAGS += -DUART1

#libraries/src/uart/uart0.rel : libraries/src/uart/uart.c
#	$(COMPILE_COMMAND)

#libraries/src/uart/uart1.rel : libraries/src/uart/uart.c
#	$(COMPILE_COMMAND)

libraries/src/uart/uart0.c : libraries/src/uart/core/uart.c
	$(CP) $< $@
	
libraries/src/uart/uart1.c : libraries/src/uart/core/uart.c
	$(CP) $< $@

