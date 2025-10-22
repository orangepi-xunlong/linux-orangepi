.PHONY: mali_base clean

all: mali_base dma_buf_test_exporter

mali_base:
	$(MAKE) -C  drivers/gpu/arm/midgard -j

dma_buf_test_exporter:
	$(MAKE) -C drivers/base/dma_buf_test_exporter -j

clean:
	$(MAKE) -C drivers/gpu/arm/midgard clean
	$(MAKE) -C drivers/base/dma_buf_test_exporter clean