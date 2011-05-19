# This library will be made by linking uart0.rel and uart1.rel.
LIB_RELS := libraries/src/uart/uart0.rel libraries/src/uart/uart1.rel

# When those rel (object) files are compiled, there will be a
# special preprocessor flag to specify which UART to use.
libraries/src/uart/uart0.rel : C_FLAGS += -DUART0
libraries/src/uart/uart1.rel : C_FLAGS += -DUART1

# The rel files will be compiled from uart0.c and uart1.c,
# which will both be copies of core/uart.c.
libraries/src/uart/uart0.c : libraries/src/uart/core/uart.c
	$(CP) $< $@
	
libraries/src/uart/uart1.c : libraries/src/uart/core/uart.c
	$(CP) $< $@

TARGETS += libraries/src/uart/uart0.c libraries/src/uart/uart1.c
