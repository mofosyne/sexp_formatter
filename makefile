
OBJ_sexp_prettify_cli = sexp_prettify_cli.o sexp_prettify.o

all: sexp_prettify_cli

sexp_prettify_cli: $(OBJ_sexp_prettify_cli)
	$(LINK.C) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	rm $(OBJ_sexp_prettify_cli)
	rm sexp_prettify_cli
