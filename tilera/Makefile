CC=tile-cc
CFLAGS=-Os -W all -W error
LDFLAGS=-l ilib -l m

SOURCES=jacobi.c data.c file.c

OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=jacobi
OUT=out.txt
PIPE=pipe
CORES=2

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

check: clean $(EXECUTABLE)
	{ time tile-monitor --batch-mode --image tile64 --upload $(EXECUTABLE) $(EXECUTABLE) --upload $(PIPE) $(PIPE) --run - $(EXECUTABLE) simple $(CORES) - --download $(PIPE) $(PIPE) --download $(PIPE) $(PIPE) ; } >> $(OUT) 2>&1

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)

purge: clean
	rm -f $(OUT)
