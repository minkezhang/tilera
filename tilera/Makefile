CC=tile-cc
CFLAGS=-W all -W error
LDFLAGS=-l ilib -l m
SOURCES=jacobi.c data.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=jacobi
OUT=test_output.txt

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

check: $(EXECUTABLE)
	time tile-monitor --batch-mode --image tile64 --upload $(EXECUTABLE) $(EXECUTABLE) --run - $(EXECUTABLE) - > $(OUT)
clean:
	rm -f $(EXECUTABLE) $(OBJECTS) $(OUT)
