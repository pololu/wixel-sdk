# This library will be made by linking spi0_master.rel and spi1_master.rel.
LIB_RELS := libraries/src/spi_master/spi0_master.rel libraries/src/spi_master/spi1_master.rel

# When those rel (object) files are compiled, there will be a
# special preprocessor flag to specify which SPI to use.
libraries/src/spi_master/spi0_master.rel : C_FLAGS += -DSPI0
libraries/src/spi_master/spi1_master.rel : C_FLAGS += -DSPI1

# The rel files will be compiled from spi0_master.c and spi1_master.c,
# which will both be copies of core/spi_master.c.
libraries/src/spi_master/spi0_master.c : libraries/src/spi_master/core/spi_master.c
	$(CP) $< $@
	
libraries/src/spi_master/spi1_master.c : libraries/src/spi_master/core/spi_master.c
	$(CP) $< $@

TARGETS += libraries/src/spi_master/spi0_master.c libraries/src/spi_master/spi1_master.c
