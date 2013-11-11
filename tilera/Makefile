CC=tile-cc
CFLAGS=-Os -W all -W error
LDFLAGS=-l ilib -l m

SOURCES=jacobi.c data.c file.c

OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=jacobi
OUT=check_out.txt
PIPE=pipe
CORES=10
RUN=run.sh
SAMPLE=sample
RESULTS=results
HEADER=data

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

check: clean $(EXECUTABLE)
	{ time tile-monitor --batch-mode --image tile64 --upload $(EXECUTABLE) $(EXECUTABLE) --upload $(PIPE) $(PIPE) --run - $(EXECUTABLE) $(SAMPLE) $(CORES) - --download $(PIPE) $(PIPE) --download $(PIPE) $(PIPE) ; } >> $(OUT) 2>&1

run: purge $(EXECUTABLE)
	python generate.py
	./$(RUN)

clean:
	rm -f $(EXECUTABLE) $(OBJECTS)

purge: clean
	rm -rf $(OUT) $(RESULTS)/* nohup.out $(PIPE)/$(HEADER)*
