all: buildtgdsmb

buildtgdsmb: 
	$(MAKE)	-R	-C	ntr/
	
clean:
	$(MAKE)	clean	-C	ntr/