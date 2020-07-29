
BINARIES=\
	test-localfile

all:	$(BINARIES)

test-localfile:	test-localfile.c common.c
	$(CC) -o $@ $< common.c

clean:
	rm -f $(BINARIES) *.o
