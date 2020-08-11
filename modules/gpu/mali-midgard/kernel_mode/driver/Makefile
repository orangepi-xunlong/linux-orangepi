.PHONY: all kds mali_base clean

all: kds mali_base dma_buf_test_exporter

kds:
	$(MAKE) -C drivers/base/kds -j

mali_base:
	$(MAKE) -C  drivers/gpu/arm/midgard -j

dma_buf_test_exporter:
	$(MAKE) -C drivers/base/dma_buf_test_exporter -j
clean:
	$(MAKE) -C drivers/base/kds clean
	$(MAKE) -C drivers/gpu/arm/midgard clean
	$(MAKE) -C drivers/base/dma_buf_test_exporter clean
