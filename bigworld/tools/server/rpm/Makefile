all: bigworld-bwmachined


bigworld-bwmachined:
	@echo "================================"
	@echo "Creating BWMachined binary RPM"
	@echo "================================"
	@echo "Preparing to build BWMachined binary RPM:"
	./generate.py bwmachined
	@echo -e "\n\n" 
	@echo "Building BWMachined binary RPM:"
	rpmbuild -bb bwmachined/bwmachined.spec
	@echo -e "\n\n"
	@echo "Performing cleanup:"
	./generate.py cleanup
	@echo 
	@echo
	@echo "Finished generating binary RPM for BWMachined."
	@echo 


clean:
	./generate.py cleanup
