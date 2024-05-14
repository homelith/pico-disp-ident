################################################################################
# Makefile for go project
################################################################################
TGT := png2array
SRC := png2array.go

# this Makefile read only first arguments as target and the rest as arguments for target
# arguments are treated as do-nothing targets from here
RUN_ARGS := $(wordlist 2,$(words $(MAKECMDGOALS)),$(MAKECMDGOALS))
$(eval $(RUN_ARGS):;@:)

.PHONY: default convert clean $(RUN_ARGS)

################################################################################

default : $(TGT)

convert : $(TGT)
	./$(TGT) -m rgb565 $(firstword $(RUN_ARGS)) > $(basename $(firstword $(RUN_ARGS)))_rgb565.h

$(TGT) : $(SRC) $(PREFIXED_GOPKGS)
	go mod tidy
	go fmt $(firstword $^)
	go build $(firstword $^)
	chmod +x $@

clean:
	rm -f $(TGT)
