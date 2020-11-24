XBE_TITLE = distance_fog_ffp
GEN_XISO = $(XBE_TITLE).iso

SRCS =  $(wildcard $(CURDIR)/src/*.c) \
        $(wildcard $(CURDIR)/../common/*.c)

SHADER_OBJS = src/combiner.inl

NXDK_DIR = $(CURDIR)/../../nxdk
NXDK_SDL = y

%.inl: %.ps | fp20compiler
	@echo "[ FP20     ] $@"
	$(FP20COMPILER) $< > $@

include $(NXDK_DIR)/Makefile
