all: buildtgdsmb

buildtgdsmb: 
	$(MAKE)	-R	-C	ntr/
	$(MAKE)	-R	-C	twl/
	
clean:
	$(MAKE)	clean	-C	ntr/
	$(MAKE)	clean	-C	twl/