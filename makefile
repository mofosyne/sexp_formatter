
all: sexp_prettify_cli sexp_prettify_cpp_cli sexp_prettify_kicad_cli sexp_prettify_kicad_original_cli

sexp_prettify_cli.o: sexp_prettify.c
	$(CC) -c -o $@ $^

sexp_prettify_cli: sexp_prettify_cli.c sexp_prettify.o sexp_prettify.h
	$(CC) -o $@ $^

sexp_prettify_cpp_cli: sexp_prettify_cpp_cli.cpp sexp_prettify.o sexp_prettify.h
	$(CXX) -o $@ $^

sexp_prettify_kicad_cli: sexp_prettify_kicad_cli.cpp
	$(CXX) -o $@ $^

sexp_prettify_kicad_original_cli: sexp_prettify_kicad_original_cli.cpp
	$(CXX) -o $@ $^

.PHONY: clean
clean:
	rm *.o  || true
	rm sexp_prettify_cli || true
	rm sexp_prettify_cpp_cli || true
	rm sexp_prettify_kicad_cli || true
	rm sexp_prettify_kicad_original_cli || true

.PHONY: test
test: all
	./test_all.sh

.PHONY: time
time: all
	./time_test_all.sh
