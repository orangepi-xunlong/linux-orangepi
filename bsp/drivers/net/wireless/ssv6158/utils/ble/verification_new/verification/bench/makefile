BENCH_UTIL := ctrl_bench.o

bench.emu_2dut.o: $(BENCH_UTIL)


bench_hci:
	@echo "Compiling bench hci"
#@cd hci_tools ; make bench_hci

%.o: %.c
	@echo "Compiling $@"
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo "Do clean ..."
	@rm -f *.o
	@cd cli ; make clean
#@cd hci_tools ; make clean
