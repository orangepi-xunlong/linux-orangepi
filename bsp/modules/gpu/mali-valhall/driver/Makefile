.PHONY: mali_base clean

all: mali_base dma_buf_test_exporter

mali_base:
	$(MAKE) -C  drivers/gpu/arm/midgard

dma_buf_test_exporter:
	$(MAKE) -C drivers/base/arm/dma_buf_test_exporter

clean:
	$(MAKE) -C drivers/gpu/arm/midgard clean
	$(MAKE) -C drivers/base/arm/dma_buf_test_exporter clean
